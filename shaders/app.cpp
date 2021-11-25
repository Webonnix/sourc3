#include "Shaders/common.h"
#include "Shaders/app_common_impl.h"

namespace Env {
#include "bvm2_cost.h"
} // namespace Env

#include "contract.h"

#include <algorithm>
#include <vector>
#include <utility>
#include <string_view>
#include <memory>

namespace GitRemoteBeam
{
#include "contract_sid.i"
}

namespace
{
    using Action_func_t = void (*)(const ContractID&);
    using Actions_map_t = std::vector<std::pair<std::string_view, Action_func_t>>;
    using Roles_map_t = std::vector<std::pair<std::string_view, const Actions_map_t&>>;

    constexpr size_t ACTION_BUF_SIZE = 32;
    constexpr size_t ROLE_BUF_SIZE = 16;

    void On_error(const char* msg)
    {
        Env::DocGroup root("");
        {
            Env::DocAddText("error", msg);
        }
    }

    template <typename T>
    auto find_if_contains(const std::string_view str, const std::vector<std::pair<std::string_view, T>>& v)
    {
        return std::find_if(v.begin(), v.end(), [&str](const auto& p) {
            return str == p.first;
        });
    }

    void On_action_create_contract(const ContractID& unused)
    {
        GitRemoteBeam::InitialParams params;

        Env::GenerateKernel(nullptr, GitRemoteBeam::InitialParams::METHOD, &params, sizeof(params), nullptr, 0, nullptr, 0, "Create GitRemoteBeam contract", 0);
    }

    void On_action_destroy_contract(const ContractID& cid)
    {
        Env::GenerateKernel(&cid, 1, nullptr, 0, nullptr, 0, nullptr, 0, "Destroy GitRemoteBeam contract", 0);
    }

    void On_action_view_contracts(const ContractID& unused)
    {
        EnumAndDumpContracts(GitRemoteBeam::s_SID);
    }

    void On_action_view_contract_params(const ContractID& cid)
    {
        Env::Key_T<int> k;
        k.m_Prefix.m_Cid = cid;
        k.m_KeyInContract = 0;

        GitRemoteBeam::InitialParams params;
        if (!Env::VarReader::Read_T(k, params))
            return On_error("Failed to read contract's initial params");

        Env::DocGroup gr("params");
    }

    void On_action_create_repo(const ContractID& cid)
    {
        using namespace GitRemoteBeam;

        char repoName[RepoInfo::MAX_NAME_SIZE + 1];
        auto nameLen = Env::DocGetText("repo_name", repoName, sizeof(repoName));
        if (nameLen <= 1) {
            return On_error("'repo_name' required");
        }
        --nameLen; // remove 0-term
        auto argsSize = sizeof(CreateRepoParams) + nameLen;
        auto buf = std::make_unique<uint8_t[]>(argsSize);
        auto* request = reinterpret_cast<CreateRepoParams*>(buf.get());
        Env::DerivePk(request->repo_owner, &cid, sizeof(cid));
        request->repo_name_length = nameLen;
        Env::Memcpy(request->repo_name, repoName, nameLen);

        SigRequest sig;
        sig.m_pID = &cid;
        sig.m_nID = sizeof(cid);

        // estimate change
        //uint32_t charge =
        //    Env::Cost::CallFar +
        //    Env::Cost::LoadVar_For(sizeof(InitialParams)) +
        //    Env::Cost::LoadVar_For(sizeof(uint64_t)) +
        //    Env::Cost::SaveVar_For(sizeof(InitialParams)) +
        //    Env::Cost::SaveVar_For(sizeof(CreateRepoParams)) +
        //    Env::Cost::SaveVar_For(sizeof(uint64_t)) +
        //    Env::Cost::AddSig +
        //    Env::Cost::MemOpPerByte * (sizeof(RepoInfo) + sizeof(CreateRepoParams)) +
        //    Env::Cost::Cycle * 300; // should be enought

        Env::GenerateKernel(&cid, CreateRepoParams::METHOD, request, argsSize,
                            nullptr, 0, &sig, 1, "create repo", 10000000);
    }

    void On_action_my_repos(const ContractID& cid) 
    {
        using namespace GitRemoteBeam;
        Env::Key_T<GeneralKey> start, end;
        start.m_KeyInContract.repo_id = 0;
        start.m_KeyInContract.op = REPO;
        _POD_(start.m_Prefix.m_Cid) = cid;
        _POD_(end) = start;
        end.m_KeyInContract.repo_id = std::numeric_limits<uint64_t>::max();

        Env::Key_T<GeneralKey> key;
        PubKey my_key;
        Env::DerivePk(my_key, &cid, sizeof(cid));
        Env::DocGroup root("");
        Env::DocArray repos("repos");
        uint32_t valueLen = 0, keyLen = sizeof(PubKey);
        for (Env::VarReader reader(start, end); reader.MoveNext(&key, keyLen, nullptr, valueLen, 0);) {
            auto buf = std::make_unique<uint8_t[]>(valueLen);
            reader.MoveNext(&key, keyLen, buf.get(), valueLen, 1);
            auto* value = reinterpret_cast<RepoInfo*>(buf.get());
            if (_POD_(value->owner) == my_key) {
                Env::DocGroup repo_object("");
                Env::DocAddNum("repo_id", key.m_KeyInContract.repo_id);
                Env::DocAddText("repo_name", value->name);
            }
            valueLen = 0;
        }
    }

    void On_action_delete_repo(const ContractID& cid) {
        uint64_t repo_id;
        if (!Env::DocGet("repo_id", repo_id)) {
            return On_error("no repo id for deleting");
        }

        GitRemoteBeam::DeleteRepoParams request;
        request.repo_id = repo_id;

        SigRequest sig;
        sig.m_pID = &cid;
        sig.m_nID = sizeof(cid);

        Env::GenerateKernel(&cid, GitRemoteBeam::DeleteRepoParams::METHOD, &request, sizeof(request),
            nullptr, 0, &sig, 1, "delete repo", 10000000);
    }

    void On_action_add_user_params(const ContractID& cid) {
        GitRemoteBeam::AddUserParams request;
        Env::DocGet("repo_id", request.repo_id);
        Env::DocGet("user", request.user);

        SigRequest sig;
        sig.m_pID = &cid;
        sig.m_nID = sizeof(cid);

        Env::GenerateKernel(&cid, GitRemoteBeam::AddUserParams::METHOD, &request, sizeof(request),
            nullptr, 0, &sig, 1, "add user params", 0);
    }

    void On_action_remove_user_params(const ContractID& cid) {
        GitRemoteBeam::RemoveUserParams request;
        Env::DocGet("repo_id", request.repo_id);
        Env::DocGet("user", request.user);

        SigRequest sig;
        sig.m_pID = &cid;
        sig.m_nID = sizeof(cid);

        Env::GenerateKernel(&cid, GitRemoteBeam::RemoveUserParams::METHOD, &request, sizeof(request),
            nullptr, 0, &sig, 1, "remove user params", 0);
    }

    void On_action_push_objects(const ContractID& cid)
    {
        using namespace GitRemoteBeam;
        auto dataLen = Env::DocGetBlob("data", nullptr, 0);
        if (!dataLen) {
            return On_error("there is no data to push");
        }

        auto argsSize = sizeof(PushObjectsParams) + dataLen;
        auto buf = std::make_unique<uint8_t[]>(argsSize);;
        auto* params = reinterpret_cast<PushObjectsParams*>(buf.get());
        if (Env::DocGetBlob("data", &params->objects_info, dataLen) != dataLen) {
            return On_error("failed to read push data");
        }
        if (!Env::DocGet("repo_id", params->repo_id)) {
            return On_error("failed to read 'repo_id'");
        }

        char refName[GitRef::MAX_NAME_SIZE + 1];
        auto nameLen = Env::DocGetText("ref", refName, _countof(refName));
        if (nameLen <= 1) {
            return On_error("failed to read 'ref'");
        }
        --nameLen; // remove '0'-term;
        size_t refsCount = 1; // single ref for now
        auto refArgsSize = sizeof(PushRefsParams) + sizeof(GitRef) + nameLen;
        auto resMemory = std::make_unique<uint8_t[]>(refArgsSize);
        auto* refsParams = reinterpret_cast<PushRefsParams*>(resMemory.get());
        refsParams->repo_id = params->repo_id;
        refsParams->refs_info.refs_number = refsCount;
        auto* ref = reinterpret_cast<GitRef*>(refsParams + 1);
        if (!Env::DocGetBlob("ref_target", &ref->commit_hash, sizeof(git_oid))) {
            return On_error("failed to read 'ref_target'");
        }
        ref->name_length = nameLen;
        Env::Memcpy(ref->name, refName, nameLen);

        // dump refs for debug
        Env::DocGroup grr("refs");
        {
            Env::DocAddNum32("count", refsParams->refs_info.refs_number);
            Env::DocGroup gr2("ref");
            Env::DocAddBlob("oid", &ref->commit_hash, 20);
            Env::DocAddText("name", ref->name);
        }

        // dump objects for debug
        Env::DocGroup gr("objects");
        {
            Env::DocAddNum32("count", params->objects_info.objects_number);

            auto* obj = reinterpret_cast<const GitObject*>(&params->objects_info + 1);
            for (uint32_t i = 0; i < params->objects_info.objects_number; ++i) {

                uint32_t size = obj->data_size;
                Env::DocGroup gr2("object");
                Env::DocAddBlob("oid", &obj->hash, sizeof(git_oid));
                Env::DocAddNum32("size", size);
                Env::DocAddNum32("type", obj->type);
                ++obj; // skip header
                obj = reinterpret_cast<const GitObject*>(reinterpret_cast<const uint8_t*>(obj) + size); // move to next object
            }
        }


        Env::DerivePk(params->user, &cid, sizeof(cid));

        SigRequest sig;
        sig.m_pID = &cid;
        sig.m_nID = sizeof(cid);

        Env::GenerateKernel(&cid, PushObjectsParams::METHOD, params, argsSize,
            nullptr, 0, &sig, 1, "Pushing objects", 20000000);

        Env::GenerateKernel(&cid, PushRefsParams::METHOD, refsParams, refArgsSize,
            nullptr, 0, &sig, 1, "Pushing refs", 10000000);
    }

    void On_action_user_get_key(const ContractID& cid)
    {
        PubKey pk;
        Env::DerivePk(pk, &cid, sizeof(cid));
        Env::DocAddBlob_T("key", pk);
    }
}


BEAM_EXPORT void Method_0()
{
    Env::DocGroup root("");
    {
        Env::DocGroup gr("roles");
        {
            Env::DocGroup grRole("manager");
            {
                Env::DocGroup grMethod("create_contract");
            }
            {
                Env::DocGroup grMethod("destroy_contract");
                Env::DocAddText("cid", "ContractID");
            }
            {
                Env::DocGroup grMethod("view_contracts");
            }
			{
				Env::DocGroup grMethod("view_contract_params");
				Env::DocAddText("cid", "ContractID");
			}
        }
        {
            Env::DocGroup grRole("user");
            {
            }
        }
    }
}

BEAM_EXPORT void Method_1()
{
	const Actions_map_t VALID_USER_ACTIONS = {
        {"create_repo", On_action_create_repo},
        {"my_repos", On_action_my_repos},
        {"delete_repo", On_action_delete_repo},
        {"add_user_params", On_action_add_user_params},
        {"remove_user_params", On_action_remove_user_params},
        {"push_objects", On_action_push_objects},
        {"get_key", On_action_user_get_key}
	};

	const Actions_map_t VALID_MANAGER_ACTIONS = {
		{"create_contract", On_action_create_contract},
		{"destroy_contract", On_action_destroy_contract},
		{"view_contracts", On_action_view_contracts},
		{"view_contract_params", On_action_view_contract_params},
	};

	/* Add your new role's actions here */

	const Roles_map_t VALID_ROLES = {
		{"user", VALID_USER_ACTIONS},
		{"manager", VALID_MANAGER_ACTIONS},
		/* Add your new role here */
	};

	char action[ACTION_BUF_SIZE], role[ROLE_BUF_SIZE];

	if (!Env::DocGetText("role", role, sizeof(role))) {
		return On_error("Role not specified");
	}

	auto it_role = find_if_contains(role, VALID_ROLES);

	if (it_role == VALID_ROLES.end()) {
		return On_error("Invalid role");
	}

	if (!Env::DocGetText("action", action, sizeof(action))) {
		return On_error("Action not specified");
	}

	auto it_action = find_if_contains(action, it_role->second);

	if (it_action != it_role->second.end()) {
		ContractID cid;
		Env::DocGet("cid", cid);
		it_action->second(cid);
	} else {
		return On_error("Invalid action");
	}
}
