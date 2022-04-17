#include "autostart.h"
#include <QDebug>

#if defined(TARGET_LINUX)

#include <string>
#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>
#include <fstream>
#include <filesystem>
#include <QFile>
#include <QSettings>
#include <QFileInfo>
#include <QDir>

namespace fs = std::filesystem;

#define QBREAK_DESKTOP_NAME "qbreak.desktop"

static std::string read_desktop_file()
{
    QFile f(":/assets/misc/qbreak.desktop");
    f.open(QFile::ReadOnly);
    auto bytes = f.readAll();
    return bytes.toStdString();
}

static std::string fixup_desktop_file(const std::string& desktop_unit, const std::string& path)
{
    // load set with first file
    std::istringstream inf(desktop_unit);
    std::vector<std::string> lines;
    std::string line;
    for (unsigned int i=0; std::getline(inf,line); i++)
    {
        if (line.find("Exec=") == 0)
        {
            line = "Exec=" + path;
        }
        lines.push_back(line);
    }

    std::ostringstream oss;
    std::copy(lines.begin(), lines.end(), std::ostream_iterator<std::string>(oss, "\n"));
    return oss.str();
}

static fs::path home_dir()
{
    struct passwd *pw = getpwuid(getuid());
    return pw ? pw->pw_dir : std::string();
}

static fs::path autostart_dir()
{
    return home_dir() / ".config" / "autostart";
}

static fs::path autostart_path()
{
    return autostart_dir() / QBREAK_DESKTOP_NAME;
}

void autostart::enable(const std::string& path_to_me)
{
    // Put .desktop file to ~/.config/autostart
    std::ofstream ofs(autostart_path());
    if (ofs.is_open())
    {
        ofs << fixup_desktop_file(read_desktop_file(), path_to_me);
        ofs.close();
    }
    else
        qDebug() << "Failed to write the desktop entry into autostart dir. Error: " << errno;
}

void autostart::disable()
{
    // Remove .desktop file from ~/.config/autostart
    ::remove(autostart_path().c_str());
}

bool autostart::is_enabled()
{
    return fs::exists(fs::path(autostart_path()));
}

// ----- appmenu -----
static fs::path appmenu_install_dir()
{
    // Global one
    // return fs::path("/usr/share/applications");

    // User only
    return home_dir() / ".local" / "share" / "applications";
}

void appmenu::install(const std::string& path_to_me)
{
    // Put .desktop file to ~/.config/autostart
    std::ofstream ofs(appmenu_install_dir() / QBREAK_DESKTOP_NAME);
    if (ofs.is_open())
    {
        ofs << fixup_desktop_file(read_desktop_file(), path_to_me);
        ofs.close();
    }
    else
        qDebug() << "Failed to write the desktop entry into app menu dir. Error: " << errno;

    // Install latest icons
    {
        auto icons = {"16x16", "32x32", "64x64", "128x128", "512x512"};

        // Here Qt part
        auto target_dir = QFileInfo(QDir::homePath() + "/.local/share/icons/hicolor").absoluteFilePath();
        if (QFileInfo::exists(target_dir))
        {
            // Copy icons from resources
            for (auto& icon_suffix: icons)
            {
                QString icon_src = QString(":/assets/images/coffee_cup/icon_") + icon_suffix + ".png",
                        icon_dst = target_dir + "/" + icon_suffix + "/apps/qbreak.png";
                QFile::copy(icon_src, icon_dst);
            }
        }
    }
}

void appmenu::uninstall()
{
    if (is_installed())
        fs::remove(appmenu_install_dir() / QBREAK_DESKTOP_NAME);
}

bool appmenu::is_installed()
{
    return fs::exists(appmenu_install_dir() / QBREAK_DESKTOP_NAME);
}
#endif
