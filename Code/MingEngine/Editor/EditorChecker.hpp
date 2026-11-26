#pragma once

#include "MingEngine/Core/IO/VirtualPath.hpp"
#include "MingEngine/Core/IO/Resource.hpp"
#include "MingEngine/Core/Math/Vector2.hpp"
#include "MingEngine/Core/OS/WorkerThreadPool.hpp"

#include <atomic>
#include <chrono>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

class EditorNode;

class EditorChecker
{
private:
	struct Stamp
	{
		bool operator==(Stamp const& other) const;

		std::filesystem::file_time_type m_time{};
		uintmax_t                       m_size = 0;
	};

	struct PendingDrop
	{
		std::vector<std::string> m_paths;
		VirtualPath              m_directory;
	};

	struct CopyFile
	{
		std::filesystem::path m_source;
		std::filesystem::path m_target;
		bool                  m_replace = false;
	};

public:
	explicit EditorChecker(EditorNode& editor);
	~EditorChecker();

	void MarkSceneDirty();
	void ClearSceneDirty();
	bool IsSceneDirty() const;

	void MarkResourceDirty(Resource* resource);
	bool IsResourceDirty(VirtualPath const& path) const;
	bool HasDirtyResources() const;
	bool SaveResource(Ref<Resource> const& resource);
	bool SaveDirtyResources();
	void ApproveQuit() { m_quitApproved = true; }
	bool IsQuitApproved() const { return m_quitApproved; }
	bool HasUnsavedChanges() const;
	bool SavePendingChanges();

	void CheckProject(bool full = false);
	void Update();
	void Shutdown();
	void NotifyFileSaved(VirtualPath const& path);
	void NotifySceneLoaded(VirtualPath const& path);
	void ImportExternal(std::vector<std::string> const& paths, Vector2 position);
	bool IsBusy() const;
	bool IsInitialScanComplete() const;

private:
	static bool ReadStamp(std::filesystem::path const& path, Stamp& stamp);
	static void ScanTask(void* userdata);
	void CopyFiles();
	void StartScan();
	void AcceptScan();
	void FinishImports();
	void CommitTree();
	void ResolveExternalChanges(bool reload);
	void StartExternalCopy(std::vector<std::string> const& paths, VirtualPath const& target);
	void PrepareCopy(std::filesystem::path const& source, std::filesystem::path const& target);
	void ContinueCopy();
	void ShowProgress(std::string const& message, int completed, int total);

private:
	EditorNode&                           m_editor;
	bool                                  m_sceneDirty = false;
	bool                                  m_quitApproved = false;
	std::map<std::string, Ref<Resource>>  m_dirtyResources;
	std::filesystem::path                 m_root;
	std::map<std::string, Stamp>          m_known;
	std::map<std::string, Stamp>          m_scanned;
	std::vector<VirtualPath>              m_directories;
	std::vector<std::string>              m_errors;
	std::vector<std::string>              m_imports;
	std::vector<std::string>              m_conflicts;
	std::vector<PendingDrop>              m_drops;
	std::vector<CopyFile>                 m_copies;
	std::vector<CopyFile>                 m_pendingCopies;
	WorkerThreadPool::TaskID              m_task = WorkerThreadPool::INVALID_TASK_ID;
	std::atomic<int>                      m_completed{0};
	std::atomic<bool>                     m_stop{false};
	std::chrono::steady_clock::time_point m_started;
	size_t                                m_importIndex = 0;
	size_t                                m_collisionIndex = 0;
	uint64_t                              m_revision = 0;
	uint64_t                              m_scanRevision = 0;
	bool                                  m_pending = true;
	bool                                  m_full = false;
	bool                                  m_scanFull = false;
	bool                                  m_importing = false;
	bool                                  m_choosing = false;
	bool                                  m_initialComplete = false;
	bool                                  m_progressVisible = false;
	bool                                  m_refreshTree = false;
};
