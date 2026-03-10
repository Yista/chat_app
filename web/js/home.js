// ==================== 配置 ====================
const API_BASE = 'http://192.168.2.23:8080';
const WS_URL = 'ws://192.168.2.23:8081';

let ws = null;
let currentUser = { id: null, username: null };
let friends = [];                // 好友列表 { id, username, online }
let currentChat = null;           // 当前聊天对象 { id, username }
let messages = [];                // 当前聊天的消息列表（用于撤回时更新）

// ==================== 工具函数 ====================
function showToast(message, type = 'info', duration = 3000) {
    let toastContainer = document.getElementById('toast');
    const toast = document.createElement('div');
    toast.className = `toast ${type} mb-2 flex items-center gap-2`;
    toast.innerHTML = `
        <span class="flex-1 text-sm text-gray-700">${message}</span>
        <button class="text-gray-400 hover:text-gray-600" onclick="this.parentElement.remove()">✕</button>
    `;
    toastContainer.appendChild(toast);
    setTimeout(() => {
        toast.style.animation = 'slideOutRight 0.3s ease-out forwards';
        setTimeout(() => toast.remove(), 300);
    }, duration);
}

// 检查登录状态
function checkAuth() {
    const id = sessionStorage.getItem('userId');
    const username = sessionStorage.getItem('username');
    const password = sessionStorage.getItem('password'); // 实际应使用token
    if (!id || !username || !password) {
        console.warn('Missing auth info, redirecting to login.html');
        window.location.href = 'login.html';
        return false;
    }
    currentUser.id = parseInt(id);
    currentUser.username = username;
    document.getElementById('currentUsername').textContent = username;
    document.getElementById('avatarInitial').textContent = username.charAt(0).toUpperCase();
    return true;
}

// 显示消息到聊天区域
function displayMessage(sender, content, isOwn, msgId, timestamp) {
    const container = document.getElementById('messageContainer');
    const msgDiv = document.createElement('div');
    msgDiv.className = `flex items-start gap-2 ${isOwn ? 'justify-end' : ''} message-enter`;
    if (isOwn) {
        msgDiv.innerHTML = `
            <span class="text-xs text-gray-400 self-end">刚刚</span>
            <div class="max-w-xs bg-blue-500 text-white rounded-2xl p-3 relative group">
                <p class="text-sm">${content}</p>
                <button class="recall-btn hidden group-hover:inline-block absolute top-0 right-0 bg-red-500 text-white text-xs px-2 py-1 rounded" data-msg-id="${msgId}">撤回</button>
            </div>
            <div class="w-8 h-8 rounded-full bg-gradient-to-r from-blue-400 to-blue-600 flex-shrink-0"></div>
        `;
        msgDiv.classList.add('own');
    } else {
        msgDiv.innerHTML = `
            <div class="w-8 h-8 rounded-full bg-gradient-to-r from-green-400 to-green-600 flex-shrink-0"></div>
            <div class="max-w-xs bg-gray-100 border border-gray-200 rounded-2xl p-3">
                <p class="text-sm text-gray-800">${content}</p>
            </div>
            <span class="text-xs text-gray-400 self-end">${sender}</span>
        `;
    }
    msgDiv.dataset.msgId = msgId;
    container.appendChild(msgDiv);
    container.scrollTop = container.scrollHeight;

    // 为自己消息添加撤回事件
    if (isOwn && msgId) {
        const recallBtn = msgDiv.querySelector('.recall-btn');
        recallBtn.addEventListener('click', () => {
            if (ws && ws.readyState === WebSocket.OPEN) {
                ws.send(JSON.stringify({ type: 'recall', msg_id: msgId }));
            } else {
                showToast('WebSocket未连接', 'error');
            }
        });
    }
}

// 加载好友列表
async function loadFriends() {
    try {
        const response = await fetch(`${API_BASE}/api/friends/list`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ user_id: currentUser.id })
        });
        const data = await response.json();
        if (Array.isArray(data)) {
            friends = data.map(f => ({ id: f.id, username: f.username, online: f.online || false }));
            renderFriendList();
        } else {
            showToast('获取好友列表失败', 'error');
        }
    } catch (err) {
        console.error('loadFriends error:', err);
    }
}

// 渲染好友列表
function renderFriendList() {
    const container = document.getElementById('friendListContainer');
    container.innerHTML = '';
    if (friends.length === 0) {
        container.innerHTML = '<div class="p-4 text-center text-gray-400">暂无好友</div>';
        return;
    }
    friends.forEach(f => {
        const div = document.createElement('div');
        div.className = `friend-item flex items-center gap-3 p-3 hover:bg-gray-100 cursor-pointer transition ${currentChat && currentChat.id === f.id ? 'bg-gray-100' : ''}`;
        div.dataset.id = f.id;
        div.dataset.username = f.username;
        div.innerHTML = `
            <div class="w-10 h-10 rounded-full bg-gradient-to-r from-purple-400 to-purple-600 flex items-center justify-center text-white font-bold">${f.username.charAt(0).toUpperCase()}</div>
            <div class="flex-1 min-w-0">
                <div class="flex justify-between">
                    <h3 class="font-medium text-gray-800">${f.username}</h3>
                    <span class="text-xs ${f.online ? 'text-green-500' : 'text-gray-400'}">${f.online ? '● 在线' : '○ 离线'}</span>
                </div>
            </div>
        `;
        div.addEventListener('click', () => selectFriend(f.id, f.username));
        container.appendChild(div);
    });
    document.getElementById('friendCount').textContent = friends.length;
}

// 选择好友聊天
function selectFriend(id, username) {
    currentChat = { id, username };
    document.getElementById('chatWith').textContent = username;
    document.getElementById('chatAvatar').textContent = username.charAt(0).toUpperCase();
    document.getElementById('messageInput').disabled = false;
    document.getElementById('sendBtn').disabled = false;
    document.getElementById('messageContainer').innerHTML = '';
    messages = [];

    // 加载历史消息
    loadHistoryMessages(id);
}

// 加载历史消息
async function loadHistoryMessages(otherId) {
    try {
        const response = await fetch(`${API_BASE}/api/messages/history`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ user_id: currentUser.id, other_id: otherId, limit: 50 })
        });
        const data = await response.json();
        if (Array.isArray(data)) {
            data.forEach(msg => {
                displayMessage(
                    msg.sender_id === currentUser.id ? '我' : msg.sender_username,
                    msg.content,
                    msg.sender_id === currentUser.id,
                    msg.id,
                    msg.created_at
                );
                messages.push(msg);
            });
        }
    } catch (err) {
        console.error('loadHistoryMessages error:', err);
    }
}

// 初始化 WebSocket
function initWebSocket() {
    ws = new WebSocket(WS_URL);
    ws.onopen = () => {
        console.log('WebSocket connected');
        ws.send(JSON.stringify({ type: 'login', username: currentUser.username, password: sessionStorage.getItem('password') }));
    };
    ws.onmessage = (event) => {
        const msg = JSON.parse(event.data);
        console.log('ws message:', msg);

        if (msg.type === 'login_result') {
            if (!msg.success) {
                showToast('WebSocket登录失败', 'error');
                ws.close();
                sessionStorage.clear();
                window.location.href = 'login.html';
            } else {
                showToast('WebSocket登录成功', 'success');
            }
        }
        else if (msg.type === 'private') {
            // 收到新消息
            if (currentChat && (msg.from_id === currentChat.id || msg.from === currentChat.username)) {
                displayMessage(msg.from, msg.content, false, msg.msg_id, msg.created_at);
                messages.push({ id: msg.msg_id, sender_id: msg.from_id, content: msg.content });
            } else {
                // 非当前聊天对象，可以显示通知或更新好友列表上的未读标记（此处简化）
                showToast(`来自 ${msg.from} 的新消息`, 'info');
            }
        }
        else if (msg.type === 'private_sent') {
            // 消息发送回执，更新消息ID
            const lastOwnMsg = document.querySelector('#messageContainer .own:last-child');
            if (lastOwnMsg) {
                lastOwnMsg.dataset.msgId = msg.msg_id;
                const recallBtn = lastOwnMsg.querySelector('.recall-btn');
                if (recallBtn) recallBtn.dataset.msgId = msg.msg_id;
            }
        }
        else if (msg.type === 'recall') {
            // 撤回通知
            const targetMsg = document.querySelector(`.message[data-msg-id="${msg.msg_id}"]`);
            if (targetMsg) {
                const isOwn = targetMsg.classList.contains('own');
                const recallText = isOwn ? '你撤回了一条消息' : `${msg.from} 撤回了一条消息`;
                targetMsg.querySelector('p').textContent = recallText;
                const recallBtn = targetMsg.querySelector('.recall-btn');
                if (recallBtn) recallBtn.remove();
                // 从本地消息列表中移除或标记
            }
        }
        else if (msg.type === 'error') {
            showToast('错误：' + msg.message, 'error');
        }
    };
    ws.onclose = () => {
        console.log('WebSocket disconnected');
        showToast('与服务器断开连接', 'error');
        // 可尝试重连
    };
    ws.onerror = (err) => {
        console.error('WebSocket error:', err);
        showToast('WebSocket连接错误', 'error');
    };
}

// 发送消息
function sendMessage() {
    const input = document.getElementById('messageInput');
    const text = input.value.trim();
    if (!text || !currentChat) return;
    if (ws && ws.readyState === WebSocket.OPEN) {
        ws.send(JSON.stringify({
            type: 'private',
            to: currentChat.username,
            content: text
        }));
        // 立即显示自己消息（等待回执更新msg_id）
        displayMessage('我', text, true, 0, Date.now());
        input.value = '';
    } else {
        showToast('WebSocket未连接', 'error');
    }
}

// 搜索用户
async function searchUsers() {
    console.log('searchUsers called');
    const keyword = document.getElementById('searchInput').value.trim();
    if (!keyword) return;
    try {
        const response = await fetch(`${API_BASE}/api/users/search`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ keyword, user_id: currentUser.id })
        });
        const data = await response.json();
        const resultsDiv = document.getElementById('searchResults');
        resultsDiv.innerHTML = '';
        if (data.length === 0) {
            resultsDiv.innerHTML = '<div class="p-2 text-gray-400">未找到用户</div>';
        } else {
            data.forEach(u => {
                const item = document.createElement('div');
                item.className = 'p-2 hover:bg-gray-100 cursor-pointer flex justify-between items-center';
                item.innerHTML = `
                    <span>${u.username} ${u.is_friend ? '(已是好友)' : ''}</span>
                    ${!u.is_friend && u.id !== currentUser.id ? '<button class="text-blue-500 text-xs add-friend">添加好友</button>' : ''}
                `;
                if (!u.is_friend && u.id !== currentUser.id) {
                    item.querySelector('.add-friend').addEventListener('click', (e) => {
                        e.stopPropagation();
                        sendFriendRequest(u.id, u.username);
                    });
                }
                resultsDiv.appendChild(item);
            });
        }
        resultsDiv.classList.remove('hidden');
    } catch (err) {
        console.error('search error:', err);
    }
}

// 发送好友请求
async function sendFriendRequest(targetId, targetUsername) {
    try {
        const response = await fetch(`${API_BASE}/api/friends/request`, {
            method: 'POST',
            headers: { 'Content-Type': 'application/json' },
            body: JSON.stringify({ user_id: currentUser.id, target_username: targetUsername })
        });
        const data = await response.json();
        if (data.status === 'ok') {
            showToast('好友请求已发送', 'success');
        } else {
            showToast('发送失败：' + data.message, 'error');
        }
    } catch (err) {
        showToast('网络错误', 'error');
    }
}

// 登出
function logout() {
    if (ws) ws.close();
    sessionStorage.clear();
    window.location.href = 'login.html';
}

// ==================== 初始化 ====================
document.addEventListener('DOMContentLoaded', () => {
    if (!checkAuth()) return;

    // 加载好友列表
    loadFriends();

    // 初始化 WebSocket
    initWebSocket();

    // 绑定事件
    document.getElementById('searchBtn').addEventListener('click', searchUsers);
    document.getElementById('sendBtn').addEventListener('click', sendMessage);
    document.getElementById('messageInput').addEventListener('keypress', (e) => {
        if (e.key === 'Enter') sendMessage();
    });
    document.getElementById('logoutBtn').addEventListener('click', logout);
    document.addEventListener('click', (e) => {
        if (!e.target.closest('#searchResults') && !e.target.closest('#searchBtn')) {
            document.getElementById('searchResults').classList.add('hidden');
        }
    });
});