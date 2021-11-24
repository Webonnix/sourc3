#include "Shaders/common.h"
#include "Shaders/app_common_impl.h"
#include "Shaders/Math.h"
#include "contract.h"

#include <algorithm>
#include <tuple>

using namespace GitRemoteBeam;

BEAM_EXPORT void Ctor(const InitialParams& params)
{
	ContractState cs;
	cs.last_repo_id = 1;
	Env::SaveVar_T(0, cs);
}

BEAM_EXPORT void Dtor(void*)
{
	Env::DelVar_T(0);
}

BEAM_EXPORT void Method_2(const CreateRepoParams& params)
{
	HashProcessor::Sha256 hash_obj;
	Hash256 repo_name_hash;
	hash_obj.Write(params.repo_name, params.repo_name_length);
	hash_obj >> repo_name_hash;

	auto key1 = RepoInfo::Key(params.repo_owner, repo_name_hash);
	uint64_t repo_id = 0;

	// halt if repo exists
	Env::Halt_if(Env::LoadVar_T(key1, repo_id) && repo_id != 0);

	ContractState cs;
	Env::LoadVar_T(0, cs);

	repo_id = cs.last_repo_id++;

	Env::SaveVar_T(0, cs);
	Env::SaveVar_T(key1, repo_id);

	RepoInfo* repo_info = static_cast<RepoInfo*>(Env::Heap_Alloc(sizeof(RepoInfo) + params.repo_name_length));

	Env::Memcpy(&repo_info->name_hash, &repo_name_hash, sizeof(repo_name_hash));
	repo_info->owner = params.repo_owner;
	repo_info->repo_id = repo_id;
	repo_info->name_length = params.repo_name_length;
	Env::Memcpy(repo_info->name, params.repo_name, repo_info->name_length);

	auto key_user = RepoUser::Key(params.repo_owner, repo_info->repo_id);
	Env::SaveVar_T(key_user, true);

	auto key_repo = std::make_pair(repo_id, Operations::REPO);
	auto key_repo_size = std::make_pair(repo_id, Operations::REPO_SIZE);
	Env::SaveVar(&key_repo, sizeof(key_repo), repo_info, sizeof(RepoInfo) + repo_info->name_length, KeyTag::Internal);
	Env::SaveVar_T(key_repo_size, sizeof(RepoInfo) + repo_info->name_length);

	Env::Heap_Free(repo_info);

	Env::AddSig(repo_info->owner);
}

BEAM_EXPORT void Method_3(const DeleteRepoParams& params)
{
	auto key_repo_size = std::make_pair(params.repo_id, Operations::REPO_SIZE);
	auto key_repo = std::make_pair(params.repo_id, Operations::REPO);
	size_t repo_size;

	Env::Halt_if(!Env::LoadVar_T(key_repo_size, repo_size));
	RepoInfo* repo_info = static_cast<RepoInfo*>(Env::Heap_Alloc(repo_size));
	
	Env::LoadVar(&key_repo, sizeof(key_repo), repo_info, repo_size, KeyTag::Internal);
	
	Env::DelVar_T(RepoInfo::Key(repo_info->owner, repo_info->name_hash));
	Env::DelVar_T(RepoUser::Key(repo_info->owner, repo_info->repo_id));
	Env::AddSig(repo_info->owner);
	for (auto op : ALL_OPERATIONS) {
		auto key = std::make_pair(params.repo_id, op);
		Env::DelVar_T(key);
	}

	Env::Heap_Free(repo_info);
}

BEAM_EXPORT void Method_4(const AddUserParams& params)
{
	auto key_repo_size = std::make_pair(params.repo_id, Operations::REPO_SIZE);
	auto key_repo = std::make_pair(params.repo_id, Operations::REPO);
	size_t repo_size;

	Env::Halt_if(!Env::LoadVar_T(key_repo_size, repo_size));
	RepoInfo* repo_info = static_cast<RepoInfo*>(Env::Heap_Alloc(repo_size));
	
	Env::LoadVar(&key_repo, sizeof(key_repo), repo_info, repo_size, KeyTag::Internal);

	auto key_user = RepoUser::Key(params.user, repo_info->repo_id);
	Env::SaveVar_T(key_user, true);

	Env::AddSig(repo_info->owner);
	Env::Heap_Free(repo_info);
}

BEAM_EXPORT void Method_5(const RemoveUserParams& params)
{
	auto key_repo_size = std::make_pair(params.repo_id, Operations::REPO_SIZE);
	auto key_repo = std::make_pair(params.repo_id, Operations::REPO);
	size_t repo_size;

	Env::Halt_if(!Env::LoadVar_T(key_repo_size, repo_size));
	RepoInfo* repo_info = static_cast<RepoInfo*>(Env::Heap_Alloc(repo_size));
	
	Env::LoadVar(&key_repo, sizeof(key_repo), repo_info, repo_size, KeyTag::Internal);

	auto key_user = RepoUser::Key(params.user, repo_info->repo_id);
	bool is_exist;

	if (Env::LoadVar_T(key_user, is_exist)) {
		Env::DelVar_T(key_user);
	}

	Env::AddSig(repo_info->owner);
	Env::Heap_Free(repo_info);
}

BEAM_EXPORT void Method_6(const PushObjectsParams& params)
{
	auto key_repo_size = std::make_pair(params.repo_id, Operations::REPO_SIZE);
	auto key_repo = std::make_pair(params.repo_id, Operations::REPO);
	size_t repo_size;

	Env::Halt_if(!Env::LoadVar_T(key_repo_size, repo_size));
	RepoInfo* repo_info = static_cast<RepoInfo*>(Env::Heap_Alloc(repo_size));
	
	Env::LoadVar(&key_repo, sizeof(key_repo), repo_info, repo_size, KeyTag::Internal);

	auto key_user = RepoUser::Key(params.user, repo_info->repo_id);
	bool is_exist;

	Env::Halt_if(!Env::LoadVar_T(key_user, is_exist));
	
	auto* obj = reinterpret_cast<const GitObject*>(&params.objects_info + 1);
	for (uint32_t i = 0; i < params.objects_info.objects_number; ++i) {
		auto key = GitObject::Key(params.repo_id, obj->hash, Operations::OBJECTS);
		Env::Halt_if(Env::LoadVar(&key, sizeof(key), nullptr, 0, KeyTag::Internal)); // halt if object exists
		Env::SaveVar(&key, sizeof(key), obj->data, obj->data_size, KeyTag::Internal);
		auto size = obj->data_size;
		++obj; // skip header
		obj = reinterpret_cast<const GitObject*>(reinterpret_cast<const uint8_t*>(obj) + size); // move to next object
	}

	Env::AddSig(params.user);
	Env::Heap_Free(repo_info);
}

BEAM_EXPORT void Method_7(const PushRefsParams& params)
{
	auto key_repo_size = std::make_pair(params.repo_id, Operations::REPO_SIZE);
	auto key_repo = std::make_pair(params.repo_id, Operations::REPO);
	size_t repo_size;

	Env::Halt_if(!Env::LoadVar_T(key_repo_size, repo_size));
	RepoInfo* repo_info = static_cast<RepoInfo*>(Env::Heap_Alloc(repo_size));
	
	Env::LoadVar(&key_repo, sizeof(key_repo), repo_info, repo_size, KeyTag::Internal);

	auto key_user = RepoUser::Key(params.user, repo_info->repo_id);
	bool is_exist;

	Env::Halt_if(!Env::LoadVar_T(key_user, is_exist));

	// TODO: replace tuple
	for (size_t i = 0; i < params.refs_info.refs_number; ++i) {
		auto key = std::make_tuple(params.repo_id, params.refs_info.refs[i].name, Operations::REFS);
		Env::SaveVar_T(key, params.refs_info.refs[i].commit_hash);
	}

	Env::AddSig(params.user);
	Env::Heap_Free(repo_info);
}
