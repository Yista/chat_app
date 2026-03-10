#ifndef MESSAGE_DAO_HPP
#define MESSAGE_DAO_HPP

#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "db_manager.hpp"
#include "models/message.hpp"

class MessageDAO {
public:
    MessageDAO(std::shared_ptr<DBManager> db);

    bool createTables();

    // 插入消息，返回生成的ID
    int insertMessage(int senderId, int receiverId, const std::string& content, int msgType = 0);

    // 查询两个用户之间的历史消息（按时间倒序）
    std::vector<Message> getHistoryMessages(int userId1, int userId2, int limit = 50);

    // 撤回消息（仅限发送者，2分钟内可撤回）
    bool recallMessage(int messageId, int senderId);

    // 软删除消息（标记为删除）
    bool deleteMessage(int messageId, int userId); // userId用于验证权限

    // 获取非好友之间的临时消息计数
    int getTempMessageCount(int userId1, int userId2);
    bool incrementTempMessageCount(int userId1, int userId2);
    bool resetTempMessageCount(int userId1, int userId2); // 成为好友后重置

private:
    std::shared_ptr<DBManager> db_;
};

#endif