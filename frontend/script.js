
// DISPLAY MESSAGES
function showMessage(text, success = false) {
    const message = document.getElementById("message");

    if (message) {
        message.textContent = text;
        message.style.color = success ? "green" : "red";
    }
}

// SEND DATA TO C++ SERVER
async function sendRequest(url, data) {
    const response = await fetch(url, {
        method: "POST",
        headers: {
            "Content-Type": "application/x-www-form-urlencoded"
        },
        body: new URLSearchParams(data)
    });

    return await response.json();
}

// ACCOUNT TYPE LABEL
function getAccountTypeLabel(type) {
    const labels = {
        "Savings": "Savings Account",
        "Current": "Current Account",
        "Fixed Deposit": "Fixed Deposit Account"
    };

    return labels[type] || "Savings Account";
}

// REGISTRATION
const registerForm = document.getElementById("registerForm");

if (registerForm) {
    registerForm.addEventListener("submit", async function(event) {
        event.preventDefault();

        const accountNumber = document.getElementById("accountNumber").value.trim();
        const accountHolder = document.getElementById("accountHolder").value.trim();
        const pin = document.getElementById("pin").value.trim();
        const accountType = document.getElementById("accountType").value;
        const message = document.getElementById("message");

        if (!/^[0-9]{4}$/.test(pin)) {
            message.textContent = "PIN must contain exactly 4 digits.";
            return;
        }

        if (!accountType) {
            message.textContent = "Please select an account type.";
            return;
        }

        try {
            const result = await sendRequest("/api/register", {
                accountNumber,
                accountHolder,
                pin,
                accountType
            });

            message.textContent = result.message;

            if (result.success) {
                registerForm.reset();
            }
        } catch (error) {
            message.textContent = "Unable to connect to the server.";
        }
    });
}

// LOGIN
const loginForm = document.getElementById("loginForm");

if (loginForm) {
    loginForm.addEventListener("submit", async function(event) {
        event.preventDefault();

        const accountNumber = document.getElementById("accountNumber").value.trim();
        const pin = document.getElementById("pin").value.trim();
        const message = document.getElementById("message");

        try {
            const result = await sendRequest("/api/login", {
                accountNumber,
                pin
            });

            message.textContent = result.message;

            if (result.success) {
                sessionStorage.setItem("accountNumber", accountNumber);
                window.location.href = "dashboard.html";
            }
        } catch (error) {
            message.textContent = "Unable to connect to the server.";
        }
    });
}

// GET LOGGED-IN ACCOUNT NUMBER
function getLoggedInAccount() {
    return sessionStorage.getItem("accountNumber");
}

// LOAD ACCOUNT DETAILS
async function loadAccountDetails() {
    const number = getLoggedInAccount();

    if (!number) {
        window.location.href = "login.html";
        return null;
    }

    const accountNumber = document.getElementById("accountNumber");

    if (accountNumber) {
        accountNumber.textContent = number;
    }

    try {
        const result = await sendRequest("/api/account", {
            accountNumber: number
        });

        if (result.success) {
            const customerName = document.getElementById("customerName");
            const dashboardAccountType = document.getElementById("dashboardAccountType");

            if (customerName) {
                customerName.textContent = result.accountHolder;
            }

            if (dashboardAccountType) {
                dashboardAccountType.textContent =
                    getAccountTypeLabel(result.accountType);
            }

            return result.accountHolder;
        }

        showMessage("Unable to load account details.");
        return null;

    } catch (error) {
        showMessage("Unable to load account details.");
        return null;
    }
}

// LOAD BALANCE
async function loadBalance() {
    const number = getLoggedInAccount();

    if (!number) return;

    try {
        const result = await sendRequest("/api/balance", {
            accountNumber: number
        });

        if (result.success) {
            const balance = Number(result.balance).toFixed(2);

            const balanceElement = document.getElementById("balance");
            const profileBalance = document.getElementById("profileBalance");

            if (balanceElement) {
                balanceElement.textContent = balance;
            }

            if (profileBalance) {
                profileBalance.textContent = balance;
            }
        } else {
            showMessage(result.message);
        }
    } catch (error) {
        showMessage("Unable to load balance.");
    }
}

// DEPOSIT MONEY
async function depositMoney() {
    const amount = Number(document.getElementById("depositAmount").value);
    const number = getLoggedInAccount();

    if (!Number.isFinite(amount) || amount <= 0) {
        showMessage("Enter a valid deposit amount.");
        return;
    }

    try {
        const result = await sendRequest("/api/deposit", {
            accountNumber: number,
            amount: amount
        });

        showMessage(result.message, result.success);

        if (result.success) {
            document.getElementById("depositAmount").value = "";
            await refreshDashboard();
        }
    } catch (error) {
        showMessage("Deposit failed. Please try again.");
    }
}

// WITHDRAW MONEY
async function withdrawMoney() {
    const amount = Number(document.getElementById("withdrawAmount").value);
    const number = getLoggedInAccount();

    if (!Number.isFinite(amount) || amount <= 0) {
        showMessage("Enter a valid withdrawal amount.");
        return;
    }

    try {
        const result = await sendRequest("/api/withdraw", {
            accountNumber: number,
            amount: amount
        });

        showMessage(result.message, result.success);

        if (result.success) {
            document.getElementById("withdrawAmount").value = "";
            await refreshDashboard();
        }
    } catch (error) {
        showMessage("Withdrawal failed. Please try again.");
    }
}

// TRANSFER MONEY
async function transferMoney() {
    const receiver = document.getElementById("receiverAccount").value.trim();
    const amount = Number(document.getElementById("transferAmount").value);
    const sender = getLoggedInAccount();

    if (!receiver) {
        showMessage("Enter the receiver account number.");
        return;
    }

    if (receiver === sender) {
        showMessage("You cannot transfer money to your own account.");
        return;
    }

    if (!Number.isFinite(amount) || amount <= 0) {
        showMessage("Enter a valid transfer amount.");
        return;
    }

    try {
        const result = await sendRequest("/api/transfer", {
            sender: sender,
            receiver: receiver,
            amount: amount
        });

        showMessage(result.message, result.success);

        if (result.success) {
            document.getElementById("receiverAccount").value = "";
            document.getElementById("transferAmount").value = "";
            await refreshDashboard();
        }
    } catch (error) {
        showMessage("Transfer failed. Please try again.");
    }
}

// LOAD TRANSACTION HISTORY
async function loadTransactions() {
    const number = getLoggedInAccount();

    if (!number) return;

    try {
        const result = await sendRequest("/api/transactions", {
            accountNumber: number
        });

        const history = document.getElementById("transactionHistory");

        if (!history) return;

        if (result.success && result.transactions.trim() !== "") {
            history.textContent = result.transactions;
        } else {
            history.textContent = "No transactions found.";
        }
    } catch (error) {
        const history = document.getElementById("transactionHistory");

        if (history) {
            history.textContent = "Unable to load transaction history.";
        }
    }
}

// REFRESH DASHBOARD
async function refreshDashboard() {
    await loadBalance();
    await loadTransactions();
}

// LOAD PROFILE DETAILS
async function loadProfile() {
    const number = getLoggedInAccount();

    if (!number) {
        window.location.href = "login.html";
        return;
    }

    try {
        const result = await sendRequest("/api/account", {
            accountNumber: number
        });

        if (!result.success) {
            window.location.href = "login.html";
            return;
        }

        document.getElementById("profileName").textContent =
            result.accountHolder;

        document.getElementById("profileHolder").textContent =
            result.accountHolder;

        document.getElementById("profileAccount").textContent =
            number;

        document.getElementById("profileAvatar").textContent =
            result.accountHolder.charAt(0).toUpperCase();

        const profileAccountType =
            document.getElementById("profileAccountType");

        if (profileAccountType) {
            profileAccountType.textContent =
                getAccountTypeLabel(result.accountType);
        }

        await loadBalance();

    } catch (error) {
        document.getElementById("profileName").textContent =
            "Unable to load profile.";
    }
}

// LOGOUT
function logout() {
    sessionStorage.removeItem("accountNumber");
    window.location.href = "login.html";
}

// INITIALIZE DASHBOARD
if (document.getElementById("customerName")) {
    if (!getLoggedInAccount()) {
        window.location.href = "login.html";
    } else {
        loadAccountDetails();
        refreshDashboard();
    }
}

// INITIALIZE PROFILE PAGE
if (document.getElementById("profileName")) {
    loadProfile();
}
