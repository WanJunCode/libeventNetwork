#include "MysqlPool.h"
#include "../main/logcpp.h"

MysqlPool::MysqlPool(const std::string &server,
                       unsigned int port,
                       const std::string &db_name,
                       const std::string &user,
                       const std::string &password,
                       unsigned int keeplive,
                       unsigned int maxConn)
    : server_(server)
    , db_(db_name)
    , user_(user)
    , password_(password)
    , port_(port)
    , keeplive_(keeplive)
    , conns_max_(maxConn){

}


MysqlPool::~MysqlPool() {
    clear();
    for (std::vector<mysqlpp::Connection*>::iterator iter = resumMysql_.begin(); iter != resumMysql_.end(); ++iter) {
        delete *iter;
    }
}

void MysqlPool::runInThread() {
    bool stat = false;
    while (true) {
        std::string sql = sqlString_.pop_front(stat);// 阻塞
        if (!stat) {
            LOG_ERROR("SQL queue empty.\n");
            continue;
        }
        mysqlpp::Connection::thread_start();
        try {
            // Go get a free connection from the pool, or create a new one
            // if there are no free conns yet.  Uses safe_grab() to get a
            // connection from the pool that will be automatically returned
            // to the pool when this loop iteration finishes.
            mysqlpp::ScopedConnection cp(*this, true);
            if (!cp) {
                LOG_ERROR("Failed to get a connection from the pool!\n");
            } else {
                // Use a higher level of transaction isolation than MySQL
                // offers by default.  This trades some speed for more
                // predictable behavior.  We've set it to affect all
                // transactions started through this DB server connection,
                // so it affects the next block, too, even if we don't
                // commit this one.
                mysqlpp::Transaction trans(*cp, mysqlpp::Transaction::serializable, mysqlpp::Transaction::session);
                // Show initial state
                mysqlpp::Query query = cp->query();

                do {
                    if (stat) {
                        if (!query.exec(sql)) {
                            LOG_WARN("execute sql[%s] failded", sql.c_str());
                        }
                        LOG_DEBUG("SQL:[%s]",sql.data());
                    }
                    if (sqlString_.size() > 0) {
                        sql = sqlString_.pop_front(stat);
                    } else {
                        break;
                    }
                } while (true);
                trans.commit();
            }
        } catch (mysqlpp::Exception& e) {
            LOG_ERROR("Failed to set up initial pooled connection: %s\n",e.what());
        }

        // Release the per-thread resources before we exit
        mysqlpp::Connection::thread_end();
    }
}

// Do a simple form of in-use connection limiting: wait to return
// a connection until there are a reasonably low number in use
// already.  Can't do this in create() because we're interested in
// connections actually in use, not those created.  Also note that
// we keep our own count; ConnectionPool::size() isn't the same!
mysqlpp::Connection* MysqlPool::grab() {
    std::unique_lock<std::mutex> locker(mutex_);
    if (conns_in_use_.get() >= conns_max_) {
        shrink();
        // indicate waiting for release
        condition_.wait(locker);
    }
    mysqlpp::Connection *conn = mysqlpp::ConnectionPool::grab();
    conns_in_use_.increment();
    LOG_DEBUG("-------increment:[%d]\n",conns_in_use_.get());
    return conn;
}

// Other half of in-use conn count limit
void MysqlPool::release(const mysqlpp::Connection* conn) {
    mysqlpp::ConnectionPool::release(conn);
    conns_in_use_.decrement();
    condition_.notify_one();
    LOG_DEBUG("-------decrement:",conns_in_use_.get());
}

// Superclass overrides
mysqlpp::Connection* MysqlPool::create() {
    // Create connection using the parameters we were passed upon
    // creation.  This could be something much more complex, but for
    // the purposes of the example, this suffices.

    std::unique_lock<std::mutex> locker(mutex_);
    while (!resumMysql_.empty()) {
        LOG_DEBUG("-------reuse\n");
        mysqlpp::Connection* conn = resumMysql_.back();
        resumMysql_.pop_back();
        if (conn->ping()) {
            LOG_DEBUG("-------reuse mysql connection\n");
            return conn;
        } else {
            LOG_DEBUG("-------clear invalid mysql connection\n");
            delete conn;
        }
    }
    LOG_DEBUG("-------create\n");
    return new mysqlpp::Connection(
               db_.empty() ? 0 : db_.c_str(),
               server_.empty() ? 0 : server_.c_str(),
               user_.empty() ? 0 : user_.c_str(),
               password_.empty() ? "" : password_.c_str(),
               port_>0 ? port_ : 0);
}

void MysqlPool::destroy(mysqlpp::Connection* connection) {
    // Our superclass can't know how we created the Connection, so
    // it delegates destruction to us, to be safe.
    LOG_DEBUG("-------return\n");
    std::lock_guard<std::mutex> locker(mutex_);
    std::vector<mysqlpp::Connection*>::iterator result = std::find(resumMysql_.begin(), resumMysql_.end(), connection);
    if (result == resumMysql_.end()) {
        resumMysql_.push_back(connection);
    }
}
