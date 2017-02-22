#include <QApplication>
#include <QDir>
#include <QMessageBox>
#include "fileassociation.hpp"
#include "version.hpp"

/* Windows file association code. */
#ifdef DV_FILE_ASSOCIATION
/* Windows-specific code used for file association. */
#include <Windows.h>
#pragma comment(lib, "advapi32")

namespace fileassociation {

void addRegistryEntry(const QString& path, const QString& value, QString& error) {
    HKEY key;

    LSTATUS result = RegCreateKeyExA(HKEY_CURRENT_USER, LPCSTR(path.toLocal8Bit().constData()), 0, nullptr,
                                     REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, nullptr, &key, nullptr);

    if (result != ERROR_SUCCESS) {
        error += "<p>Error creating key \"" + path + "\"!</p>";
        return;
    }

    result = RegSetValueExA(key, nullptr, 0, REG_SZ, LPBYTE(value.toLocal8Bit().constData()), DWORD(value.size()));

    if (result != ERROR_SUCCESS)
        error += "<p>Error setting value for \"" + path + "\"!</p>";

    RegCloseKey(key);
}

void registerFileTypes() {
    QString error;

    QString progID = "chipgw.DepthView." + version::number.toString();

    addRegistryEntry("Software\\Classes\\.jps", progID, error);
    addRegistryEntry("Software\\Classes\\.pns", progID, error);

    /* TODO - Add video and standard image formats in a way that doesn't override the main association. */

    addRegistryEntry("Software\\Classes\\" + progID, "Stereo 3D Image", error);

    QString command = "\"" + QDir::toNativeSeparators(QApplication::applicationFilePath()) + "\" \"%1\"";
    addRegistryEntry("Software\\Classes\\" + progID + "\\shell\\open\\command", command, error);

    if (error.isNull())
        QMessageBox::information(nullptr, QObject::tr("Success!"), QObject::tr("Successfully associated .jps and .pns files with DepthView."));
    else
        QMessageBox::warning(nullptr, QObject::tr("Error setting file association!"), error);
}

}

#endif
