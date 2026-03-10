const API_BASE = 'http://192.168.2.23:8080'; // 后端API地址，根据实际情况修改

// Toast 提示函数
function showToast(message, type = 'info', duration = 3000) {
    let toastContainer = document.getElementById('toast');
    if (!toastContainer) {
        toastContainer = document.createElement('div');
        toastContainer.id = 'toast';
        toastContainer.className = 'fixed top-5 right-5 z-50';
        document.body.appendChild(toastContainer);
    }
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

// 面板切换逻辑
const loginPanel = document.getElementById('loginPanel');
const registerPanel = document.getElementById('registerPanel');
const switchToLogin = document.getElementById('switchToLogin');
const switchToRegister = document.getElementById('switchToRegister');
const authTitle = document.getElementById('authTitle');
const authSubtitle = document.getElementById('authSubtitle');

function setActivePanel(panel) {
    if (panel === 'login') {
        loginPanel.classList.remove('hidden');
        registerPanel.classList.add('hidden');
        switchToLogin.classList.add('border-blue-500', 'text-blue-600');
        switchToLogin.classList.remove('border-transparent', 'text-gray-600');
        switchToRegister.classList.remove('border-blue-500', 'text-blue-600');
        switchToRegister.classList.add('border-transparent', 'text-gray-600');
        authTitle.textContent = 'Welcome Back';
        authSubtitle.textContent = '使用你的账号继续';
    } else {
        loginPanel.classList.add('hidden');
        registerPanel.classList.remove('hidden');
        switchToRegister.classList.add('border-blue-500', 'text-blue-600');
        switchToRegister.classList.remove('border-transparent', 'text-gray-600');
        switchToLogin.classList.remove('border-blue-500', 'text-blue-600');
        switchToLogin.classList.add('border-transparent', 'text-gray-600');
        authTitle.textContent = 'Join Us';
        authSubtitle.textContent = '创建新账号';
    }
}

switchToLogin.addEventListener('click', () => setActivePanel('login'));
switchToRegister.addEventListener('click', () => setActivePanel('register'));
setActivePanel('login');

// 登录表单处理
const loginForm = document.getElementById('loginForm');
if (loginForm) {
    loginForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = document.getElementById('username').value;
        const password = document.getElementById('password').value;
        const btn = document.getElementById('loginBtn');
        btn.classList.add('pointer-events-none', 'opacity-75');
        btn.querySelector('span:first-child').classList.add('hidden');
        btn.querySelector('.loading-spinner').classList.remove('hidden');

        try {
            const response = await fetch(`${API_BASE}/api/login`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password })
            });
            const data = await response.json();
            if (data.status === 'ok') {
                // 登录成功，保存用户名和密码到 sessionStorage（临时方案，后续应使用token）
                sessionStorage.setItem('userId', data.user_id);
                sessionStorage.setItem('username', username);
                sessionStorage.setItem('password', password);
                showToast('登录成功！', 'success');
                setTimeout(() => {
                    window.location.href = 'home.html';
                }, 1000);
            } else {
                showToast('登录失败：' + (data.message || '未知错误'), 'error');
            }
        } catch (err) {
            showToast('网络错误：' + err, 'error');
        } finally {
            btn.classList.remove('pointer-events-none', 'opacity-75');
            btn.querySelector('span:first-child').classList.remove('hidden');
            btn.querySelector('.loading-spinner').classList.add('hidden');
        }
    });
}

// 注册表单处理
const registerForm = document.getElementById('registerForm');
if (registerForm) {
    registerForm.addEventListener('submit', async (e) => {
        e.preventDefault();
        const username = document.getElementById('regUsername').value;
        const password = document.getElementById('regPassword').value;
        const confirm = document.getElementById('confirmPwd').value;

        if (password !== confirm) {
            showToast('两次密码不一致', 'error');
            return;
        }

        const btn = document.getElementById('registerBtn');
        btn.classList.add('pointer-events-none', 'opacity-75');

        try {
            const response = await fetch(`${API_BASE}/api/register`, {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ username, password })
            });
            const data = await response.json();
            if (data.status === 'ok') {
                showToast('注册成功，请登录', 'success');
                setTimeout(() => {
                    setActivePanel('login');
                    // 清空注册表单
                    document.getElementById('regUsername').value = '';
                    document.getElementById('regPassword').value = '';
                    document.getElementById('confirmPwd').value = '';
                }, 1500);
            } else {
                showToast('注册失败：' + (data.message || '未知错误'), 'error');
            }
        } catch (err) {
            showToast('网络错误：' + err, 'error');
        } finally {
            btn.classList.remove('pointer-events-none', 'opacity-75');
        }
    });
}