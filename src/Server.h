#ifndef Server_h
#define Server_h

#include <FileSystemWatcher.h>
#include <ByteArray.h>
#include <List.h>
#include <Map.h>
#include "Indexer.h"
#include "Database.h"
#include "QueryMessage.h"
#include "Connection.h"
#include "ThreadPool.h"
#include "Job.h"
#include "RTags.h"
#include "ScopedDB.h"
#include "EventReceiver.h"
#include "MakefileInformation.h"

class Connection;
class Indexer;
class Message;
class ErrorMessage;
class OutputMessage;
class MakefileMessage;
class LocalServer;
class Database;
class GccArguments;
class MakefileParser;
class Completions;
class Server : public EventReceiver
{
public:
    Server();
    ~Server();
    void clear();
    enum Option {
        NoOptions = 0x0,
        NoClangIncludePath = 0x1,
        UseDashB = 0x2,
        NoValidateOnStartup = 0x4
    };
    enum DatabaseType {
        Symbol,
        SymbolName,
        Dependency,
        FileInformation,
        PCHUsrMaps,
        General,
        FileIds,
        DatabaseTypeCount,
        ProjectSpecificDatabaseTypeCount = PCHUsrMaps + 1
    };

    static Server *instance() { return sInstance; }
    List<ByteArray> defaultArguments() const { return mOptions.defaultArguments; }
    ScopedDB db(DatabaseType type, ReadWriteLock::LockType lockType, const Path &path = Path()) const;
    struct Options {
        Options() : options(0), cacheSizeMB(0), maxCompletionUnits(0) {}
        unsigned options;
        List<ByteArray> defaultArguments;
        long cacheSizeMB;
        Path socketPath;
        int maxCompletionUnits;
    };
    bool init(const Options &options);
    Indexer *indexer() const;
    ByteArray name() const { return mOptions.socketPath; }
    static bool setBaseDirectory(const Path &base, bool clear);
    Path databaseDir(DatabaseType type);
    static Path pchDir();
    static Path projectsPath();
    ThreadPool *threadPool() const { return mThreadPool; }
    void onNewConnection();
    signalslot::Signal2<int, const List<ByteArray> &> &complete() { return mComplete; }
    void startJob(Job *job);
    bool setCurrentProject(const Path &path);
protected:
    void onJobsComplete(Indexer *indexer);
    void event(const Event *event);
    void onFileReady(const GccArguments &file, MakefileParser *parser);
    void onNewMessage(Message *message, Connection *conn);
    void onIndexingDone(int id);
    void onConnectionDestroyed(Connection *o);
    void onMakefileParserDone(MakefileParser *parser);
    void onMakefileModified(const Path &path);
    void onMakefileRemoved(const Path &path);
    void make(const Path &path, List<ByteArray> makefileArgs = List<ByteArray>(),
              const List<ByteArray> &extraFlags = List<ByteArray>(), Connection *conn = 0);
private:
    struct Project;
    Project *initProject(const Path &path);
    static Path::VisitResult projectsVisitor(const Path &path, void *);
    void handleMakefileMessage(MakefileMessage *message, Connection *conn);
    void handleQueryMessage(QueryMessage *message, Connection *conn);
    void handleErrorMessage(ErrorMessage *message, Connection *conn);
    void handleCreateOutputMessage(CreateOutputMessage *message, Connection *conn);
    void fixIts(const QueryMessage &query, Connection *conn);
    void errors(const QueryMessage &query, Connection *conn);
    int followLocation(const QueryMessage &query);
    int cursorInfo(const QueryMessage &query);
    int referencesForLocation(const QueryMessage &query);
    int referencesForName(const QueryMessage &query);
    int findSymbols(const QueryMessage &query);
    int listSymbols(const QueryMessage &query);
    int status(const QueryMessage &query);
    int test(const QueryMessage &query);
    int runTest(const QueryMessage &query);
    int nextId();
    void reindex(const ByteArray &pattern);
    void remake(const ByteArray &pattern = ByteArray(), Connection *conn = 0);
    ByteArray completions(const QueryMessage &query);
private:
    static Server *sInstance;
    Options mOptions;
    LocalServer *mServer;
    Map<int, Connection*> mPendingIndexes;
    Map<int, Connection*> mPendingLookups;
    bool mVerbose;
    int mJobId;
    static Path sBase;
    Map<Path, MakefileInformation> mMakefiles;
    FileSystemWatcher mMakefilesWatcher;
    Database *mDBs[DatabaseTypeCount - ProjectSpecificDatabaseTypeCount];
    struct Project {
        Project()
            : indexer(0)
        {
            memset(databases, 0, sizeof(databases));
        }

        ~Project()
        {
            delete indexer;
            for (int i=0; i<ProjectSpecificDatabaseTypeCount; ++i) {
                delete databases[i];
            }
        }
        Path projectPath;
        Database *databases[ProjectSpecificDatabaseTypeCount];
        Indexer *indexer;
    };

    Map<Path, Project*> mProjects;
    Project *mCurrentProject;
    ThreadPool *mThreadPool;
    signalslot::Signal2<int, const List<ByteArray> &> mComplete;
    Completions *mCompletions;
};

#endif
