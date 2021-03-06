#pragma once

#include <QValidator>

class DVFolderListing;

class DVFileValidator : public QValidator {
    Q_OBJECT

    Q_PROPERTY(DVFolderListing* folderListing MEMBER folderListing NOTIFY folderListingChanged)
    Q_PROPERTY(bool filterSurround MEMBER filterSurround NOTIFY filterSurroundChanged)
    Q_PROPERTY(bool filterStereo MEMBER filterStereo NOTIFY filterStereoChanged)
    Q_PROPERTY(bool filterVideo MEMBER filterVideo NOTIFY filterVideoChanged)
    Q_PROPERTY(bool filterImage MEMBER filterImage NOTIFY filterImageChanged)
    Q_PROPERTY(bool filterDir MEMBER filterDir NOTIFY filterDirChanged)

public:
    DVFileValidator(QObject* parent = nullptr);

    State validate(QString& input, int& pos) const override;

    /* Needed for checking file properties. */
    DVFolderListing* folderListing;

    bool filterSurround;
    bool filterStereo;
    bool filterVideo;
    bool filterImage;
    bool filterDir;

signals:
    /* These are unused in C++ but QML wants them. */
    void folderListingChanged();
    void filterSurroundChanged();
    void filterStereoChanged();
    void filterVideoChanged();
    void filterImageChanged();
    void filterDirChanged();
};
