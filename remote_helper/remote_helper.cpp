﻿
#define _CRT_SECURE_NO_WARNINGS // getenv
#include "object_collector.h"
#include "utils.h"
#include "wallet_client.h"

#include <boost/filesystem.hpp>
#include <boost/program_options.hpp>
#include <boost/json.hpp>
#include <boost/algorithm/hex.hpp>

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <sstream>
#include <stack>
#include <string_view>
#include <string>
#include <vector>
#include <optional>
#include "version.h"

namespace po = boost::program_options;
namespace json = boost::json;
using namespace std;
using namespace sourc3;

#define PROTO_NAME "sourc3"

namespace
{
    constexpr size_t IPFS_ADDRESS_SIZE = 46;

    class ProgressReporter
    {
    public:
        ProgressReporter(std::string_view title, size_t total)
            : m_title(title)
            , m_total(total)
        {
            UpdateProgress(0);
        }

        ~ProgressReporter()
        {
            if (m_failureReason.empty())
            {
                Done();
            }
            else
            {
                StopProgress(m_failureReason);
            }
        }

        void UpdateProgress(size_t done)
        {
            m_done = done;
            ShowProgress("\r");
        }

        void Done()
        {
            StopProgress("done");
        }

        void Failed(const std::string& failure)
        {
            m_failureReason = failure;
        }

        void StopProgress(std::string_view result)
        {
            std::stringstream ss;
            ss << ", " << result << ".\n";
            ShowProgress(ss.str());
        }

    private:

        void ShowProgress(std::string_view eol)
        {
            std::stringstream ss;
            ss << m_title << ": ";
            if (m_total > 0)
            {
                size_t percent = m_done * 100 / m_total;
                ss << percent << "%" << " (" << m_done << "/" << m_total << ")";
            }
            else
            {
                ss << m_done;
            }
            ss << eol;
            cerr << ss.str();
            cerr.flush();
        }
    private:
        std::string_view m_title;
        std::string m_failureReason;
        size_t m_done = 0;
        size_t m_total;
    };

    template<typename String>
    ByteBuffer FromHex(const String& s)
    {
        ByteBuffer res;
        res.reserve(s.size() / 2);
        boost::algorithm::unhex(s.begin(), s.end(), std::back_inserter(res));
        return res;
    }
}

class RemoteHelper
{
public:

    RemoteHelper(SimpleWalletClient& wc)
        : m_walletClient{wc}
    {

    }

    int DoCommand(string_view command, vector<string_view>& args)
    {
        auto it = find_if(begin(m_commands), end(m_commands),
            [&](const auto& c)
            {
                return command == c.command;
            });
        if (it == end(m_commands))
        {
            cerr << "Unknown command: " << command << endl;
            return -1;
        }
        return std::invoke(it->action, this, args);
    }

    int DoList([[maybe_unused]] const vector<string_view>& args)
    {
        auto refs = RequestRefs();

        for (const auto& r : refs)
        {
            cout << to_string(r.target) << " " << r.name << '\n';
        }
        if (!refs.empty())
        {
            cout << "@" << refs.back().name << " HEAD\n";
        }
        cout << endl;
        return 0;
    }

    int DoOption([[maybe_unused]] const vector<string_view>& args)
    {
        static string_view results[] =
        {
            "error invalid value",
            "ok",
            "unsupported"
        };

        auto res = m_options.Set(args[1], args[2]);
        
        cout << results[size_t(res)];
        cout << endl;
        return 0;
    }

    int DoFetch(const vector<string_view>& args)
    {
        std::set<std::string> objectHashes;
        objectHashes.emplace(args[1].data(), args[1].size());
        size_t depth = 1;
        std::set<std::string> receivedObjects;

        auto enuqueObject = [&](const std::string& oid)
        {
            if (receivedObjects.find(oid) == receivedObjects.end())
            {
                objectHashes.insert(oid);
            }
        };

        git::RepoAccessor accessor(m_walletClient.GetRepoDir());
        size_t totalObjects = 0;
        std::vector<GitObject> objects;
        {
            auto progress = MakeProgress("Enumerating objects", 0);
            // hack Collect objects metainfo
            auto res = m_walletClient.InvokeWallet("role=user,action=repo_get_meta");
            auto root = json::parse(res);

            for (auto& objVal : root.as_object()["objects"].as_array())
            {
                if (progress)
                    progress->UpdateProgress(++totalObjects);

                auto& o = objects.emplace_back();
                auto& obj = objVal.as_object();
                o.data_size = obj["object_size"].to_number<uint32_t>();
                o.type = static_cast<int8_t>(obj["object_type"].to_number<uint32_t>());
                std::string s = obj["object_hash"].as_string().c_str();
                git_oid_fromstr(&o.hash, s.c_str());
                if (git_odb_exists(*accessor.m_odb, &o.hash))
                    receivedObjects.insert(s);
            }
        }

        auto progress = MakeProgress("Receiving objects", totalObjects - receivedObjects.size());

        size_t done = 0;
        while (!objectHashes.empty())
        {
            auto itToReceive = objectHashes.begin();
            const auto& objectToReceive = *itToReceive;
            std::stringstream ss;
            ss << "role=user,action=repo_get_data,obj_id=" << objectToReceive;

            auto res = m_walletClient.InvokeWallet(ss.str());
            auto root = json::parse(res);
            git_oid oid;
            git_oid_fromstr(&oid, objectToReceive.data());

            auto it = std::find_if(objects.begin(), objects.end(),
                [&](auto&& o) { return o.hash == oid; });
            if (it == objects.end())
            {
                //cerr << "No object data for " << objectToReceive << ", possibly submodule" << endl;
                receivedObjects.insert(objectToReceive); // move to received
                objectHashes.erase(itToReceive);

                continue;
            }
            //cerr << "Received data for:  " << objectToReceive << endl;
            receivedObjects.insert(objectToReceive);

            auto data = root.as_object()["object_data"].as_string();

            ByteBuffer buf;
            if (it->IsIPFSObject())
            {
                auto hash = FromHex(data);
                auto responce = m_walletClient.LoadObjectFromIPFS(std::string(hash.cbegin(), hash.cend()));
                auto r = json::parse(responce);
                if (r.as_object().find("result") == r.as_object().end())
                {
                    cerr << "message: " << r.as_object()["error"].as_object()["message"].as_string() <<
                        "\ndata:    " << r.as_object()["error"].as_object()["data"].as_string() << endl;
                    return -1;
                }
                auto d = r.as_object()["result"].as_object()["data"].as_array();
                buf.reserve(d.size());
                for (auto&& v : d)
                {
                    buf.emplace_back(static_cast<uint8_t>(v.get_int64()));
                }
            }
            else
            {
                buf = FromHex(data);
            }

            git_oid res_oid;
            auto type = it->GetObjectType();
            git_oid r;
            git_odb_hash(&r, buf.data(), buf.size(), type);
            if (r != oid)
            {
                // invalid hash
                return -1;
            }
            git_odb_write(&res_oid, *accessor.m_odb, buf.data(), buf.size(), type);
            if (type == GIT_OBJECT_TREE)
            {
                git::Tree tree;
                git_tree_lookup(tree.Addr(), *accessor.m_repo, &oid);

                auto count = git_tree_entrycount(*tree);
                for (size_t i = 0; i < count; ++i)
                {
                    auto* entry = git_tree_entry_byindex(*tree, i);
                    auto s = to_string(*git_tree_entry_id(entry));
                    enuqueObject(s);
                }
            }
            else if (type == GIT_OBJECT_COMMIT)
            {
                git::Commit commit;
                git_commit_lookup(commit.Addr(), *accessor.m_repo, &oid);
                if (depth < m_options.depth || m_options.depth == Options::InfiniteDepth)
                {
                    auto count = git_commit_parentcount(*commit);
                    for (unsigned i = 0; i < count; ++i)
                    {
                        auto* id = git_commit_parent_id(*commit, i);
                        auto s = to_string(*id);
                        enuqueObject(s);
                    }
                    ++depth;
                }
                enuqueObject(to_string(*git_commit_tree_id(*commit)));
            }
            if (progress)
                progress->UpdateProgress(++done);

            objectHashes.erase(itToReceive);
        }

        cout << endl;
        return 0;
    }

    int DoPush(const vector<string_view>& args)
    {
        ObjectCollector collector(m_walletClient.GetRepoDir());
        std::vector<Refs> refs;
        std::vector<git_oid> localRefs;
        for (size_t i = 1; i < args.size(); ++i)
        {
            auto& arg = args[i];
            auto p = arg.find(':');
            auto& r = refs.emplace_back();
            r.localRef = arg.substr(0, p);
            r.remoteRef = arg.substr(p + 1);
            git::Reference localRef;
            if (git_reference_lookup(localRef.Addr(), *collector.m_repo, r.localRef.c_str()) < 0)
            {
                cerr << "Local reference \'" << r.localRef << "\' doesn't exist" << endl;
                return -1;
            }
            auto& lr = localRefs.emplace_back();
            git_oid_cpy(&lr, git_reference_target(*localRef));
        }

        auto uploadedObjects = GetUploadedObjects();
        auto remoteRefs = RequestRefs();
        std::vector<git_oid> mergeBases;
        for (const auto& remoteRef : remoteRefs)
        {
            for (const auto& localRef : localRefs)
            {
                auto& base = mergeBases.emplace_back();
                git_merge_base(&base, *collector.m_repo, &remoteRef.target, &localRef);
            }
        }

        collector.Traverse(refs, mergeBases);

        auto& objs = collector.m_objects;
        std::sort(objs.begin(), objs.end(), [](auto&& left, auto&& right) {return left.oid < right.oid; });
        {
            auto it = std::unique(objs.begin(), objs.end(), [](auto&& left, auto& right) { return left.oid == right.oid; });
            objs.erase(it, objs.end());
        }

        for (auto& obj : collector.m_objects)
        {
            if (uploadedObjects.find(obj.oid) != uploadedObjects.end())
            {
                obj.selected = true;
            }
        }

        {
            auto it = std::remove_if(objs.begin(), objs.end(),
                [](const auto& o)
                {
                    return o.selected;
                });
            objs.erase(it, objs.end());
        }

        {
            auto progress = MakeProgress("Uploading objects to IPFS", collector.m_objects.size());
            size_t i = 0;
            for (auto& obj : collector.m_objects)
            {
                if (obj.selected)
                    continue;

                if (obj.GetSize() > IPFS_ADDRESS_SIZE)
                {
                    auto res = m_walletClient.SaveObjectToIPFS(obj.GetData(), obj.GetSize());
                    auto r = json::parse(res);
                    auto hashStr = r.as_object()["result"].as_object()["hash"].as_string();
                    obj.ipfsHash = ByteBuffer(hashStr.cbegin(), hashStr.cend());
                }
                if (progress)
                    progress->UpdateProgress(++i);
            }
        }

        std::sort(objs.begin(), objs.end(), [](auto&& left, auto&& right) {return left.GetSize() > right.GetSize(); });

        {
            auto progress = MakeProgress("Uploading metadata to blockchain", objs.size());
            collector.Serialize([&](const auto& buf, size_t done)
                {
                    std::stringstream ss;
                    if (!buf.empty())
                    {
                        //// log
                        //{
                        //    const auto* p = reinterpret_cast<const ObjectsInfo*>(buf.data());
                        //    const auto* cur = reinterpret_cast<const GitObject*>(p + 1);
                        //    for (uint32_t i = 0; i < p->objects_number; ++i)
                        //    {
                        //        size_t s = cur->data_size;
                        //        std::cerr << to_string(cur->hash) << '\t' << s << '\t' << (int)cur->type << '\n';
                        //        ++cur;
                        //        cur = reinterpret_cast<const GitObject*>(reinterpret_cast<const uint8_t*>(cur) + s);
                        //    }
                        //    std::cerr << std::endl;
                        //}
                        auto strData = ToHex(buf.data(), buf.size());

                        ss << "role=user,action=push_objects,data="
                            << strData;
                    }

                    if (progress)
                        progress->UpdateProgress(done);

                    bool last = (done == objs.size());
                    if (last)
                    {
                        ss << ',';
                        for (const auto& r : collector.m_refs)
                        {
                            ss << "ref=" << r.name << ",ref_target=" << ToHex(&r.target, sizeof(r.target));
                        }
                    }
                    m_walletClient.InvokeWallet(ss.str());
                });
        }
        {
            auto progress = MakeProgress("Waiting for the transaction completion", m_walletClient.GetTransactionCount());

            auto res = m_walletClient.WaitForCompletion([&](size_t d, const auto& error)
                {
                    if (progress)
                    {
                        if (error.empty())
                        {
                            progress->UpdateProgress(d);
                        }
                        else
                        {
                            progress->Failed(error);
                        }
                    }
                });
            cout << (res ? "ok " : "error ")
                 << refs[0].remoteRef << '\n';
        }
        

        cout << endl;
        return 0;
    }

    int DoCapabilities([[maybe_unused]] const vector<string_view>& args)
    {
        for (auto ib = begin(m_commands) + 1, ie = end(m_commands); ib != ie; ++ib)
        {
            cout << ib->command << '\n';
        }
        cout << endl;
        return 0;
    }
private:

    std::optional<ProgressReporter> MakeProgress(std::string_view title, size_t total)
    {
        if (m_options.progress)
            return std::optional<ProgressReporter>(std::in_place, title, total);

        return {};
    }

    std::vector<Ref> RequestRefs()
    {
        std::stringstream ss;
        ss << "role=user,action=list_refs";

        std::vector<Ref> refs;
        auto res = m_walletClient.InvokeWallet(ss.str());
        if (!res.empty())
        {
            auto root = json::parse(res);
            for (auto& rv : root.as_object()["refs"].as_array())
            {
                auto& ref = refs.emplace_back();
                auto& r = rv.as_object();
                ref.name = r["name"].as_string().c_str();
                git_oid_fromstr(&ref.target, r["commit_hash"].as_string().c_str());
            }
        }
        return refs;
    }

    std::set<git_oid> GetUploadedObjects()
    {
        std::set<git_oid> uploadedObjects;
        
        auto progress = MakeProgress("Enumerating uploaded objects", 0);
        // hack Collect objects metainfo
        auto res = m_walletClient.InvokeWallet("role=user,action=repo_get_meta");
        auto root = json::parse(res);
        for (auto& obj : root.as_object()["objects"].as_array())
        {
            auto s = obj.as_object()["object_hash"].as_string();
            git_oid oid;
            git_oid_fromstr(&oid, s.c_str());
            uploadedObjects.insert(oid);
            if (progress)
                progress->UpdateProgress(uploadedObjects.size());
        }
        return uploadedObjects;
    }
private:
    SimpleWalletClient& m_walletClient;

    typedef int (RemoteHelper::*Action)(const vector<string_view>& args);

    struct Command
    {
        string_view command;
        Action action;
    };

    Command m_commands[5] =
    {
        {"capabilities",	&RemoteHelper::DoCapabilities},
        {"list",			&RemoteHelper::DoList },
        {"option",			&RemoteHelper::DoOption},
        {"fetch",			&RemoteHelper::DoFetch},
        {"push",			&RemoteHelper::DoPush}
    };

    struct Options
    {
        enum struct SetResult
        {
            InvalidValue,
            Ok,
            Unsupported
        };

        static constexpr uint32_t InfiniteDepth = (uint32_t)std::numeric_limits<int32_t>::max();
        bool          progress = true;
        long          verbosity = 0;
        uint32_t      depth = InfiniteDepth;

        SetResult Set(string_view option, string_view value)
        {
            if (option == "progress")
            {
                if (value == "true")
                {
                    progress = true;
                }
                else if (value == "false")
                {
                    progress = false;
                }
                else
                {
                    return SetResult::InvalidValue;
                }
                return SetResult::Ok;
            }
/*          else if (option == "verbosity")
            {
                char* endPos;
                auto v = std::strtol(value.data(), &endPos, 10);
                if (endPos == value.data())
                {
                    return SetResult::InvalidValue;
                }
                verbosity = v;
                return SetResult::Ok;
            }
            else if (option == "depth")
            {
                char* endPos;
                auto v = std::strtoul(value.data(), &endPos, 10);
                if (endPos == value.data())
                {
                    return SetResult::InvalidValue;
                }
                depth = v;
                return SetResult::Ok;
            }
*/
            return SetResult::Unsupported;
        }
    };

    Options m_options;
};

int main(int argc, char* argv[])
{
    if (argc != 3)
    {
        cerr << "USAGE: git-remote-sourc3 <remote> <url>" << endl;
        return -1;
    }
    try
    {
        SimpleWalletClient::Options options;
        po::options_description desc("PIT config options");

        desc.add_options()
            ("api-host", po::value<std::string>(&options.apiHost)->default_value("localhost"), "Wallet API host")
            ("api-port", po::value<std::string>(&options.apiPort)->default_value("10000"), "Wallet API port")
            ("api-target", po::value<std::string>(&options.apiTarget)->default_value("/api/wallet"), "Wallet API target")
            ("app-shader-file", po::value<string>(&options.appPath)->default_value("app.wasm"), "Path to the app shader file")
            ("use-ipfs", po::value<bool>(&options.useIPFS)->default_value(true), "Use IPFS to store large blobs")
            ;
        po::variables_map vm;
#ifdef WIN32
        const auto* homeDir = std::getenv("USERPROFILE");
#else
        const auto* homeDir = std::getenv("HOME");
#endif
        std::string configPath = PROTO_NAME "-remote.cfg";
        if (homeDir)
        {
            configPath = std::string(homeDir) + "/." PROTO_NAME "/" + configPath;
        }
        cerr << "Reading config from: " << configPath << "..." << endl;
        const auto fullPath = boost::filesystem::system_complete(configPath).string();
        std::ifstream cfg(fullPath);
        if (cfg)
        {
            po::store(po::parse_config_file(cfg, desc), vm);
        }
        vm.notify();
        string_view sv(argv[2]);
        const string_view SCHEMA = PROTO_NAME "://";
        sv = sv.substr(SCHEMA.size());
        auto delimiterOwnerNamePos = sv.find('/');
        options.repoOwner = sv.substr(0, delimiterOwnerNamePos);
        options.repoName = sv.substr(delimiterOwnerNamePos + 1);
        auto* gitDir = std::getenv("GIT_DIR"); // set during clone
        if (gitDir)
        {
            options.repoPath = gitDir;
        }
        cerr << "     Remote: " << argv[1]
             << "\n        URL: " << argv[2]
             << "\nWorking dir: " << boost::filesystem::current_path()
             << "\nRepo folder: " << options.repoPath
             << endl;
        SimpleWalletClient walletClient{ options };
        RemoteHelper helper{ walletClient };
        git::Init init;
        string input;
        while (getline(cin, input, '\n'))
        {
            string_view args_sv(input.data(), input.size());
            vector<string_view> args;
            while (!args_sv.empty())
            {
                auto p = args_sv.find(' ');
                auto ss = args_sv.substr(0, p);
                args_sv.remove_prefix(p == string_view::npos ? ss.size() : ss.size() + 1);
                if (!ss.empty())
                {
                    args.emplace_back(ss);
                }
            }
            if (args.empty())
                return 0;

            cerr << "Command: " << input << endl;

            if (helper.DoCommand(args[0], args) == -1)
            {
                return -1;
            }
        }
    }
    catch (const exception& ex)
    {
        cerr << "Error: " << ex.what() << endl;
        return -1;
    }

    return 0;
}
