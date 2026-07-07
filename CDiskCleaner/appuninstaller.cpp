#include "appuninstaller.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>

#include <algorithm>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace {

#ifdef Q_OS_WIN
QString fromWideString(const wchar_t *text)
{
    return text ? QString::fromWCharArray(text) : QString();
}

QString queryRegistryValue(HKEY rootKey, const QString &subKey, const QString &valueName)
{
    HKEY hKey = nullptr;
    const std::wstring subKeyW = subKey.toStdWString();
    const std::wstring valueNameW = valueName.toStdWString();

    if (RegOpenKeyExW(rootKey, subKeyW.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return QString();
    }

    DWORD type = 0;
    DWORD dataSize = 0;
    LONG querySize = RegQueryValueExW(hKey, valueNameW.c_str(), nullptr, &type, nullptr, &dataSize);
    if (querySize != ERROR_SUCCESS || dataSize == 0) {
        RegCloseKey(hKey);
        return QString();
    }

    QByteArray buffer;
    buffer.resize(static_cast<int>(dataSize));
    if (RegQueryValueExW(hKey, valueNameW.c_str(), nullptr, &type,
                         reinterpret_cast<LPBYTE>(buffer.data()), &dataSize) != ERROR_SUCCESS) {
        RegCloseKey(hKey);
        return QString();
    }

    RegCloseKey(hKey);

    if (type == REG_SZ || type == REG_EXPAND_SZ) {
        return QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
    }
    if (type == REG_DWORD && dataSize >= sizeof(DWORD)) {
        return QString::number(*reinterpret_cast<const DWORD *>(buffer.constData()));
    }
    return QString();
}

bool queryRegistryDword(HKEY rootKey, const QString &subKey, const QString &valueName, DWORD *value)
{
    HKEY hKey = nullptr;
    const std::wstring subKeyW = subKey.toStdWString();
    const std::wstring valueNameW = valueName.toStdWString();

    if (RegOpenKeyExW(rootKey, subKeyW.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return false;
    }

    DWORD type = 0;
    DWORD data = 0;
    DWORD dataSize = sizeof(DWORD);
    const LONG result = RegQueryValueExW(hKey, valueNameW.c_str(), nullptr, &type,
                                         reinterpret_cast<LPBYTE>(&data), &dataSize);
    RegCloseKey(hKey);

    if (result != ERROR_SUCCESS || type != REG_DWORD) {
        return false;
    }

    if (value) {
        *value = data;
    }
    return true;
}

QStringList enumRegistrySubKeys(HKEY rootKey, const QString &subKey)
{
    QStringList keys;
    HKEY hKey = nullptr;
    const std::wstring subKeyW = subKey.toStdWString();

    if (RegOpenKeyExW(rootKey, subKeyW.c_str(), 0, KEY_READ, &hKey) != ERROR_SUCCESS) {
        return keys;
    }

    wchar_t keyName[512];
    DWORD index = 0;
    DWORD nameLen = sizeof(keyName) / sizeof(wchar_t);

    while (RegEnumKeyExW(hKey, index, keyName, &nameLen, nullptr, nullptr, nullptr, nullptr) == ERROR_SUCCESS) {
        keys.append(fromWideString(keyName));
        ++index;
        nameLen = sizeof(keyName) / sizeof(wchar_t);
    }

    RegCloseKey(hKey);
    return keys;
}

bool deleteRegistryTree(HKEY rootKey, const QString &subKey)
{
    const std::wstring subKeyW = subKey.toStdWString();
    return RegDeleteTreeW(rootKey, subKeyW.c_str()) == ERROR_SUCCESS;
}
#endif

QString normalizePath(const QString &path)
{
    return QDir::fromNativeSeparators(path).trimmed();
}

QString readRegistryStringImpl(const QString &keyPath, const QString &valueName)
{
#ifdef Q_OS_WIN
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    QString subKey = keyPath;

    if (keyPath.startsWith(QStringLiteral("HKLM\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_LOCAL_MACHINE;
        subKey = keyPath.mid(5);
    } else if (keyPath.startsWith(QStringLiteral("HKCU\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_CURRENT_USER;
        subKey = keyPath.mid(5);
    }

    return queryRegistryValue(rootKey, subKey, valueName);
#else
    Q_UNUSED(keyPath)
    Q_UNUSED(valueName)
    return QString();
#endif
}

bool readRegistryDword(const QString &keyPath, const QString &valueName, DWORD *value)
{
#ifdef Q_OS_WIN
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    QString subKey = keyPath;

    if (keyPath.startsWith(QStringLiteral("HKLM\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_LOCAL_MACHINE;
        subKey = keyPath.mid(5);
    } else if (keyPath.startsWith(QStringLiteral("HKCU\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_CURRENT_USER;
        subKey = keyPath.mid(5);
    }

    return queryRegistryDword(rootKey, subKey, valueName, value);
#else
    Q_UNUSED(keyPath)
    Q_UNUSED(valueName)
    Q_UNUSED(value)
    return false;
#endif
}

InstalledApp readAppFromRegistryKeyImpl(const QString &rootPath, const QString &subKey)
{
    InstalledApp app;
    app.registryPath = rootPath;
    app.registryKey = subKey;

    const QString keyPath = rootPath + QStringLiteral("\\") + subKey;
    app.displayName = readRegistryStringImpl(keyPath, QStringLiteral("DisplayName")).trimmed();
    app.displayVersion = readRegistryStringImpl(keyPath, QStringLiteral("DisplayVersion")).trimmed();
    app.publisher = readRegistryStringImpl(keyPath, QStringLiteral("Publisher")).trimmed();
    app.installLocation = normalizePath(readRegistryStringImpl(keyPath, QStringLiteral("InstallLocation")));
    app.uninstallString = readRegistryStringImpl(keyPath, QStringLiteral("UninstallString")).trimmed();
    app.quietUninstallString = readRegistryStringImpl(keyPath, QStringLiteral("QuietUninstallString")).trimmed();

    DWORD systemComponent = 0;
    if (readRegistryDword(keyPath, QStringLiteral("SystemComponent"), &systemComponent)) {
        app.isSystemComponent = (systemComponent == 1);
    }

    DWORD noRemove = 0;
    if (readRegistryDword(keyPath, QStringLiteral("NoRemove"), &noRemove)) {
        app.noRemove = (noRemove == 1);
    }

    DWORD estimatedSizeKb = 0;
    if (readRegistryDword(keyPath, QStringLiteral("EstimatedSize"), &estimatedSizeKb)) {
        app.estimatedSizeBytes = static_cast<qint64>(estimatedSizeKb) * 1024;
    }

    return app;
}

void enumerateFromRegistryRoot(HKEY rootKey, const QString &rootPath, QVector<InstalledApp> &apps)
{
#ifdef Q_OS_WIN
    const QString subRoot = rootPath.mid(rootPath.indexOf('\\') + 1);
    const QStringList subKeys = enumRegistrySubKeys(rootKey, subRoot);
    for (const QString &subKey : subKeys) {
        const InstalledApp app = readAppFromRegistryKeyImpl(rootPath, subKey);
        if (app.displayName.isEmpty() || app.isSystemComponent) {
            continue;
        }
        apps.append(app);
    }
#else
    Q_UNUSED(rootKey)
    Q_UNUSED(rootPath)
    Q_UNUSED(apps)
#endif
}

} // namespace

QString AppUninstaller::readRegistryString(const QString &keyPath, const QString &valueName)
{
    return readRegistryStringImpl(keyPath, valueName);
}

InstalledApp AppUninstaller::readAppFromRegistryKey(const QString &rootPath, const QString &subKey)
{
    return readAppFromRegistryKeyImpl(rootPath, subKey);
}

QVector<InstalledApp> AppUninstaller::enumerateInstalledApps()
{
    QVector<InstalledApp> apps;

#ifdef Q_OS_WIN
    enumerateFromRegistryRoot(HKEY_LOCAL_MACHINE,
                              QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                              apps);
    enumerateFromRegistryRoot(HKEY_LOCAL_MACHINE,
                              QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                              apps);
    enumerateFromRegistryRoot(HKEY_CURRENT_USER,
                              QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
                              apps);

    std::sort(apps.begin(), apps.end(), [](const InstalledApp &a, const InstalledApp &b) {
        return a.displayName.compare(b.displayName, Qt::CaseInsensitive) < 0;
    });

    apps.erase(std::unique(apps.begin(), apps.end(), [](const InstalledApp &a, const InstalledApp &b) {
                   return a.displayName.compare(b.displayName, Qt::CaseInsensitive) == 0
                          && a.displayVersion == b.displayVersion;
               }),
               apps.end());
#endif

    return apps;
}

bool AppUninstaller::isProtectedApp(const InstalledApp &app)
{
    const QString name = app.displayName.toLower();
    static const QStringList protectedNames = {
        QStringLiteral("microsoft visual c++"),
        QStringLiteral(".net"),
        QStringLiteral("windows sdk"),
        QStringLiteral("windows software development kit"),
        QStringLiteral("directx"),
        QStringLiteral("microsoft edge"),
        QStringLiteral("windows defender"),
        QStringLiteral("microsoft store")
    };

    for (const QString &item : protectedNames) {
        if (name.contains(item)) {
            return true;
        }
    }

    const QString location = app.installLocation.toLower();
    if (location.startsWith(QStringLiteral("c:/windows"))
        || location.startsWith(QStringLiteral("c:/program files/windows"))
        || location.startsWith(QStringLiteral("c:/program files (x86)/windows"))) {
        return true;
    }

    return false;
}

bool AppUninstaller::terminateRelatedProcesses(const InstalledApp &app)
{
#ifdef Q_OS_WIN
    const QString installPath = normalizePath(app.installLocation).toLower();
    const QString appName = app.displayName.section(QLatin1Char(' '), 0, 0).toLower();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) {
        return false;
    }

    PROCESSENTRY32W entry = {};
    entry.dwSize = sizeof(PROCESSENTRY32W);
    bool killedAny = false;

    if (Process32FirstW(snapshot, &entry)) {
        do {
            const DWORD pid = entry.th32ProcessID;
            if (pid == 0) {
                continue;
            }

            HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION | PROCESS_TERMINATE | PROCESS_VM_READ,
                                         FALSE, pid);
            if (!process) {
                continue;
            }

            wchar_t exePathBuffer[MAX_PATH * 4] = {};
            DWORD size = static_cast<DWORD>(sizeof(exePathBuffer) / sizeof(wchar_t));
            bool shouldKill = false;

            if (QueryFullProcessImageNameW(process, 0, exePathBuffer, &size)) {
                const QString exePath = normalizePath(fromWideString(exePathBuffer)).toLower();
                if (!installPath.isEmpty() && exePath.startsWith(installPath)) {
                    shouldKill = true;
                } else if (!appName.isEmpty()) {
                    const QString exeName = QFileInfo(exePath).completeBaseName().toLower();
                    if (exeName.contains(appName) || appName.contains(exeName)) {
                        shouldKill = true;
                    }
                }
            }

            if (shouldKill) {
                if (TerminateProcess(process, 0)) {
                    killedAny = true;
                }
            }

            CloseHandle(process);
        } while (Process32NextW(snapshot, &entry));
    }

    CloseHandle(snapshot);
    return killedAny;
#else
    Q_UNUSED(app)
    return false;
#endif
}

bool AppUninstaller::runUninstallCommand(const QString &command, int timeoutMs)
{
    if (command.isEmpty()) {
        return false;
    }

    QString program;
    QStringList arguments;
    const QString trimmed = command.trimmed();

    if (trimmed.startsWith(QLatin1Char('"'))) {
        const int endQuote = trimmed.indexOf(QLatin1Char('"'), 1);
        if (endQuote > 1) {
            program = trimmed.mid(1, endQuote - 1);
            const QString rest = trimmed.mid(endQuote + 1).trimmed();
            if (!rest.isEmpty()) {
                arguments = QProcess::splitCommand(rest);
            }
        }
    }

    if (program.isEmpty()) {
        const QStringList parts = QProcess::splitCommand(trimmed);
        if (parts.isEmpty()) {
            return false;
        }
        program = parts.first();
        arguments = parts.mid(1);
    }

    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.start();
    if (!process.waitForStarted(10000)) {
        return false;
    }

    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(3000);
        return false;
    }

    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
}

bool AppUninstaller::removeInstallDirectory(const QString &path)
{
    if (path.isEmpty()) {
        return true;
    }

    QDir dir(path);
    if (!dir.exists()) {
        return true;
    }

    return dir.removeRecursively();
}

bool AppUninstaller::removeRegistryKey(const QString &keyPath)
{
#ifdef Q_OS_WIN
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    QString subKey = keyPath;

    if (keyPath.startsWith(QStringLiteral("HKLM\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_LOCAL_MACHINE;
        subKey = keyPath.mid(5);
    } else if (keyPath.startsWith(QStringLiteral("HKCU\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_CURRENT_USER;
        subKey = keyPath.mid(5);
    } else {
        return false;
    }

    return deleteRegistryTree(rootKey, subKey);
#else
    Q_UNUSED(keyPath)
    return false;
#endif
}

UninstallResult AppUninstaller::uninstallApp(const InstalledApp &app, bool force)
{
    UninstallResult result;

    if (!app.canUninstall()) {
        result.message = QStringLiteral("该软件没有可用的卸载信息。");
        return result;
    }

    if (isProtectedApp(app)) {
        result.message = QStringLiteral("该软件属于系统或关键组件，已阻止卸载。");
        return result;
    }

    terminateRelatedProcesses(app);

    bool uninstallOk = false;
    QString command = app.quietUninstallString;
    if (command.isEmpty()) {
        command = app.uninstallString;
    }

    if (!command.isEmpty()) {
        uninstallOk = runUninstallCommand(command, force ? 120000 : 180000);
        if (!uninstallOk && force && !app.quietUninstallString.isEmpty()) {
            uninstallOk = runUninstallCommand(app.uninstallString, 180000);
        }
    }

    bool dirRemoved = false;
    bool regRemoved = false;

    if (force) {
        terminateRelatedProcesses(app);
        if (!app.installLocation.isEmpty()) {
            dirRemoved = removeInstallDirectory(app.installLocation);
        }
        if (!app.registryPath.isEmpty() && !app.registryKey.isEmpty()) {
            regRemoved = removeRegistryKey(app.registryPath + QStringLiteral("\\") + app.registryKey);
        }
    }

    if (uninstallOk || dirRemoved || regRemoved) {
        result.success = true;
        if (force) {
            result.message = QStringLiteral("强制卸载完成：%1").arg(app.displayName);
        } else {
            result.message = QStringLiteral("卸载完成：%1").arg(app.displayName);
        }
        return result;
    }

    result.message = QStringLiteral("卸载失败：%1。可尝试关闭相关程序后使用强制卸载。").arg(app.displayName);
    return result;
}

QString AppUninstaller::formatSize(qint64 bytes)
{
    if (bytes <= 0) {
        return QStringLiteral("--");
    }

    const double kb = 1024.0;
    const double mb = kb * 1024.0;
    const double gb = mb * 1024.0;

    if (bytes >= static_cast<qint64>(gb)) {
        return QStringLiteral("%1 GB").arg(bytes / gb, 0, 'f', 2);
    }
    if (bytes >= static_cast<qint64>(mb)) {
        return QStringLiteral("%1 MB").arg(bytes / mb, 0, 'f', 2);
    }
    if (bytes >= static_cast<qint64>(kb)) {
        return QStringLiteral("%1 KB").arg(bytes / kb, 0, 'f', 2);
    }
    return QStringLiteral("%1 B").arg(bytes);
}
