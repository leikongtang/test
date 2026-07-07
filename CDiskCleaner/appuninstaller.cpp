#include "appuninstaller.h"

#include <QDir>
#include <QDirIterator>
#include <QFileInfo>
#include <QProcess>
#include <QSet>
#include <QtConcurrent/QtConcurrent>

#include <algorithm>
#include <functional>

#ifdef Q_OS_WIN
#include <windows.h>
#include <tlhelp32.h>
#endif

namespace {

QString normalizePath(const QString &path)
{
    return QDir::fromNativeSeparators(path).trimmed();
}

#ifdef Q_OS_WIN
QString fromWideString(const wchar_t *text)
{
    return text ? QString::fromWCharArray(text) : QString();
}

QString queryValueFromKey(HKEY hKey, const wchar_t *valueName)
{
    if (!hKey) {
        return QString();
    }

    DWORD type = 0;
    DWORD dataSize = 0;
    if (RegQueryValueExW(hKey, valueName, nullptr, &type, nullptr, &dataSize) != ERROR_SUCCESS
        || dataSize == 0) {
        return QString();
    }

    QByteArray buffer;
    buffer.resize(static_cast<int>(dataSize));
    if (RegQueryValueExW(hKey, valueName, nullptr, &type,
                         reinterpret_cast<LPBYTE>(buffer.data()), &dataSize) != ERROR_SUCCESS) {
        return QString();
    }

    if (type == REG_SZ || type == REG_EXPAND_SZ) {
        return QString::fromWCharArray(reinterpret_cast<const wchar_t *>(buffer.constData()));
    }
    if (type == REG_DWORD && dataSize >= sizeof(DWORD)) {
        return QString::number(*reinterpret_cast<const DWORD *>(buffer.constData()));
    }
    return QString();
}

bool queryDwordFromKey(HKEY hKey, const wchar_t *valueName, DWORD *value)
{
    if (!hKey || !value) {
        return false;
    }

    DWORD type = 0;
    DWORD data = 0;
    DWORD dataSize = sizeof(DWORD);
    if (RegQueryValueExW(hKey, valueName, nullptr, &type,
                         reinterpret_cast<LPBYTE>(&data), &dataSize) != ERROR_SUCCESS
        || type != REG_DWORD) {
        return false;
    }

    *value = data;
    return true;
}

InstalledApp readAppFromOpenKey(HKEY hKey, const QString &rootPath, const QString &subKey, bool listOnly)
{
    InstalledApp app;
    app.registryPath = rootPath;
    app.registryKey = subKey;

    app.displayName = queryValueFromKey(hKey, L"DisplayName").trimmed();
    if (app.displayName.isEmpty()) {
        return app;
    }

    DWORD systemComponent = 0;
    if (queryDwordFromKey(hKey, L"SystemComponent", &systemComponent) && systemComponent == 1) {
        app.isSystemComponent = true;
        return app;
    }

    app.displayVersion = queryValueFromKey(hKey, L"DisplayVersion").trimmed();
    app.publisher = queryValueFromKey(hKey, L"Publisher").trimmed();
    app.installLocation = normalizePath(queryValueFromKey(hKey, L"InstallLocation"));
    app.displayIcon = queryValueFromKey(hKey, L"DisplayIcon").trimmed();

    if (!listOnly) {
        app.uninstallString = queryValueFromKey(hKey, L"UninstallString").trimmed();
        app.quietUninstallString = queryValueFromKey(hKey, L"QuietUninstallString").trimmed();

        DWORD noRemove = 0;
        if (queryDwordFromKey(hKey, L"NoRemove", &noRemove)) {
            app.noRemove = (noRemove == 1);
        }
    }

    DWORD estimatedSizeKb = 0;
    if (queryDwordFromKey(hKey, L"EstimatedSize", &estimatedSizeKb)) {
        app.estimatedSizeBytes = static_cast<qint64>(estimatedSizeKb) * 1024;
    }

    return app;
}

void enumerateFromRegistryRootIncremental(HKEY rootKey,
                                          const QString &rootPath,
                                          bool listOnly,
                                          const std::function<void(const InstalledApp &)> &callback)
{
    const QString subRoot = rootPath.mid(rootPath.indexOf(QLatin1Char('\\')) + 1);
    const std::wstring subRootW = subRoot.toStdWString();

    HKEY hParent = nullptr;
    if (RegOpenKeyExW(rootKey, subRootW.c_str(), 0, KEY_READ, &hParent) != ERROR_SUCCESS) {
        return;
    }

    DWORD index = 0;
    while (true) {
        DWORD nameLen = 0;
        LONG result = RegEnumKeyExW(hParent, index, nullptr, &nameLen, nullptr, nullptr, nullptr, nullptr);
        if (result == ERROR_NO_MORE_ITEMS) {
            break;
        }
        if (result != ERROR_SUCCESS && result != ERROR_MORE_DATA) {
            ++index;
            continue;
        }

        ++nameLen;
        QVector<wchar_t> keyNameBuffer(static_cast<int>(nameLen), L'\0');
        nameLen = static_cast<DWORD>(keyNameBuffer.size());
        result = RegEnumKeyExW(hParent, index, keyNameBuffer.data(), &nameLen, nullptr, nullptr, nullptr, nullptr);
        if (result != ERROR_SUCCESS) {
            ++index;
            continue;
        }

        HKEY hAppKey = nullptr;
        if (RegOpenKeyExW(hParent, keyNameBuffer.data(), 0, KEY_READ, &hAppKey) == ERROR_SUCCESS) {
            const QString subKey = fromWideString(keyNameBuffer.data());
            const InstalledApp app = readAppFromOpenKey(hAppKey, rootPath, subKey, listOnly);
            if (!app.displayName.isEmpty() && !app.isSystemComponent) {
                callback(app);
            }
            RegCloseKey(hAppKey);
        }

        ++index;
    }

    RegCloseKey(hParent);
}

void enumerateFromRegistryRoot(HKEY rootKey, const QString &rootPath, QVector<InstalledApp> &apps)
{
    enumerateFromRegistryRootIncremental(rootKey, rootPath, true, [&](const InstalledApp &app) {
        apps.append(app);
    });
}

bool deleteRegistryTree(HKEY rootKey, const QString &subKey)
{
    const std::wstring subKeyW = subKey.toStdWString();
    return RegDeleteTreeW(rootKey, subKeyW.c_str()) == ERROR_SUCCESS;
}
#endif

QString makeAppDedupKey(const InstalledApp &app)
{
    if (app.isAppxPackage) {
        return QStringLiteral("appx:") + app.packageFullName.toLower();
    }
    return app.registryPath.toLower() + QChar(0x1F) + app.registryKey.toLower();
}

QVector<InstalledApp> collectAppxPackages()
{
    QVector<InstalledApp> apps;

    QProcess process;
    const QString script = QStringLiteral(
        "Get-AppxPackage | Where-Object { -not $_.IsFramework -and $_.InstallLocation } | "
        "ForEach-Object {"
        "  $dn = $_.Name;"
        "  $manifest = Join-Path $_.InstallLocation 'AppxManifest.xml';"
        "  if (Test-Path $manifest) {"
        "    try {"
        "      $xml = [xml](Get-Content $manifest);"
        "      $val = $xml.Package.Properties.DisplayName;"
        "      if ($val -and $val -notlike 'ms-resource:*') { $dn = $val }"
        "    } catch {}"
        "  }"
        "  $dn + [char]31 + $_.Version.ToString() + [char]31 + $_.Publisher + [char]31 + "
        "  $_.InstallLocation + [char]31 + $_.PackageFullName"
        "}");
    process.start(QStringLiteral("powershell"),
                  {QStringLiteral("-NoProfile"),
                   QStringLiteral("-ExecutionPolicy"),
                   QStringLiteral("Bypass"),
                   QStringLiteral("-Command"),
                   script});

    if (!process.waitForFinished(120000)) {
        process.kill();
        return apps;
    }

    const QString output = QString::fromUtf8(process.readAllStandardOutput());
    const QStringList lines = output.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        const QString trimmed = line.trimmed();
        if (trimmed.isEmpty()) {
            continue;
        }

        const QStringList fields = trimmed.split(QChar(0x1F));
        if (fields.size() < 5) {
            continue;
        }

        InstalledApp app;
        app.isAppxPackage = true;
        app.displayName = fields.at(0).trimmed();
        app.displayVersion = fields.at(1).trimmed();
        app.publisher = fields.at(2).trimmed();
        app.installLocation = normalizePath(fields.at(3));
        app.packageFullName = fields.at(4).trimmed();
        if (app.displayName.isEmpty() || app.packageFullName.isEmpty()) {
            continue;
        }
        apps.append(app);
    }

    return apps;
}

QVector<InstalledApp> mergeAndSortApps(QVector<InstalledApp> apps)
{
    QVector<InstalledApp> uniqueApps;
    uniqueApps.reserve(apps.size());
    QSet<QString> seenKeys;

    for (const InstalledApp &app : apps) {
        const QString dedupKey = makeAppDedupKey(app);
        if (seenKeys.contains(dedupKey)) {
            continue;
        }
        seenKeys.insert(dedupKey);
        uniqueApps.append(app);
    }

    std::sort(uniqueApps.begin(), uniqueApps.end(), [](const InstalledApp &a, const InstalledApp &b) {
        return a.displayName.compare(b.displayName, Qt::CaseInsensitive) < 0;
    });

    return uniqueApps;
}

#ifdef Q_OS_WIN
QVector<InstalledApp> collectAppsFromRegistryRoot(HKEY rootKey, const QString &rootPath)
{
    QVector<InstalledApp> apps;
    enumerateFromRegistryRootIncremental(rootKey, rootPath, true, [&apps](const InstalledApp &app) {
        apps.append(app);
    });
    return apps;
}
#endif

} // namespace

InstalledApp AppUninstaller::readAppFromRegistryKey(const QString &rootPath, const QString &subKey)
{
#ifdef Q_OS_WIN
    HKEY rootKey = HKEY_LOCAL_MACHINE;
    QString regSubKey = rootPath.mid(rootPath.indexOf(QLatin1Char('\\')) + 1)
                        + QStringLiteral("\\") + subKey;
    if (rootPath.startsWith(QStringLiteral("HKCU\\"), Qt::CaseInsensitive)) {
        rootKey = HKEY_CURRENT_USER;
        regSubKey = rootPath.mid(5) + QStringLiteral("\\") + subKey;
    } else if (rootPath.startsWith(QStringLiteral("HKLM\\"), Qt::CaseInsensitive)) {
        regSubKey = rootPath.mid(5) + QStringLiteral("\\") + subKey;
    }

    HKEY hAppKey = nullptr;
    const std::wstring regSubKeyW = regSubKey.toStdWString();
    if (RegOpenKeyExW(rootKey, regSubKeyW.c_str(), 0, KEY_READ, &hAppKey) != ERROR_SUCCESS) {
        return InstalledApp();
    }

    const InstalledApp app = readAppFromOpenKey(hAppKey, rootPath, subKey, false);
    RegCloseKey(hAppKey);
    return app;
#else
    Q_UNUSED(rootPath)
    Q_UNUSED(subKey)
    return InstalledApp();
#endif
}

QVector<InstalledApp> AppUninstaller::enumerateInstalledApps()
{
#ifdef Q_OS_WIN
    const QStringList registryRoots = {
        QStringLiteral("HKLM\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKLM\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKCU\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall"),
        QStringLiteral("HKCU\\SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall")
    };

    QVector<QFuture<QVector<InstalledApp>>> futures;
    futures.reserve(registryRoots.size());
    for (const QString &rootPath : registryRoots) {
        futures.append(QtConcurrent::run([rootPath]() {
            HKEY rootKey = rootPath.startsWith(QStringLiteral("HKCU\\"), Qt::CaseInsensitive)
                               ? HKEY_CURRENT_USER
                               : HKEY_LOCAL_MACHINE;
            return collectAppsFromRegistryRoot(rootKey, rootPath);
        }));
    }

    QFuture<QVector<InstalledApp>> futureAppx = QtConcurrent::run([]() {
        return collectAppxPackages();
    });

    QVector<InstalledApp> apps;
    apps.reserve(512);
    for (QFuture<QVector<InstalledApp>> &future : futures) {
        apps += future.result();
    }
    apps += futureAppx.result();

    return mergeAndSortApps(apps);
#else
    return QVector<InstalledApp>();
#endif
}

void AppUninstaller::enumerateInstalledAppsIncremental(const std::function<void(const InstalledApp &)> &callback)
{
    const QVector<InstalledApp> apps = enumerateInstalledApps();
    for (const InstalledApp &app : apps) {
        callback(app);
    }
}

InstalledApp AppUninstaller::loadFullAppDetails(const InstalledApp &app)
{
    if (app.isAppxPackage) {
        return app;
    }

    if (!app.uninstallString.isEmpty() || !app.quietUninstallString.isEmpty()) {
        return app;
    }

    if (app.registryPath.isEmpty() || app.registryKey.isEmpty()) {
        return app;
    }

    return readAppFromRegistryKey(app.registryPath, app.registryKey);
}

bool AppUninstaller::isProtectedApp(const InstalledApp &app)
{
    const QString name = app.displayName.toLower();
    if (app.isAppxPackage) {
        const QString pkg = app.packageFullName.toLower();
        static const QStringList protectedPrefixes = {
            QStringLiteral("microsoft.windows."),
            QStringLiteral("microsoft.desktopappinstaller"),
            QStringLiteral("microsoft.storepurchaseapp"),
            QStringLiteral("windows.immersivecontrolpanel"),
            QStringLiteral("microsoft.windowsstore"),
            QStringLiteral("microsoft.windows.defender")
        };
        for (const QString &prefix : protectedPrefixes) {
            if (pkg.startsWith(prefix) || name.contains(prefix)) {
                return true;
            }
        }
    }

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

    if (command.startsWith(QStringLiteral("appx:"))) {
        const QString packageFullName = command.mid(5);
        if (packageFullName.isEmpty()) {
            return false;
        }

        QString escapedPackage = packageFullName;
        escapedPackage.replace(QLatin1Char('\''), QLatin1String("''"));

        QProcess process;
        process.start(QStringLiteral("powershell"),
                      {QStringLiteral("-NoProfile"),
                       QStringLiteral("-ExecutionPolicy"),
                       QStringLiteral("Bypass"),
                       QStringLiteral("-Command"),
                       QStringLiteral("Remove-AppxPackage -Package '%1'").arg(escapedPackage)});
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

    if (app.isAppxPackage) {
        terminateRelatedProcesses(app);
        const bool uninstallOk = runUninstallCommand(
            QStringLiteral("appx:%1").arg(app.packageFullName), force ? 120000 : 180000);
        if (uninstallOk) {
            result.success = true;
            result.message = QStringLiteral("卸载完成：%1").arg(app.displayName);
            return result;
        }
        result.message = QStringLiteral("卸载失败：%1。可尝试关闭相关程序后重试。").arg(app.displayName);
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
