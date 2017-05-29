#include "dvfilevalidator.hpp"
#include "dvfolderlisting.hpp"

DVFileValidator::DVFileValidator(QObject *parent): QValidator(parent), folderListing(nullptr) {
    filterSurround = false;
    filterStereo = false;
    filterVideo = false;
    filterImage = false;
    filterDir = false;
}

QValidator::State DVFileValidator::validate(QString& input, int& pos) const {
    QFileInfo file(input);

    /* If the parent of the path up to where the cursor is does not exist it's invalid.
     * (Prevents user from editing later parts of the path if the first part is wrong.) */
    if (!QFileInfo(input.left(pos)).absoluteDir().exists())
        return Invalid;

    if (!file.exists())
        return Intermediate;

    if (filterDir && !file.isDir())
        return Intermediate;

    /* No valid folderListing means we can't run checks. */
    if (folderListing != nullptr) {
        /* If filterSurround is set and the file isn't surround, state is Intermediate. */
        if (filterSurround && !folderListing->isFileSurround(file))
            return Intermediate;

        /* If filterStereo is set and the file isn't stereo, state is Intermediate. */
        if (filterStereo && (folderListing->fileStereoMode(file) == DVSourceMode::Mono))
            return Intermediate;

        /* If filterImage is set and the file isn't an image, state is Intermediate. */
        if (filterImage && !folderListing->isFileImage(file))
            return Intermediate;

        /* If filterVideo is set and the file isn't a video, state is Intermediate. */
        if (filterVideo && !folderListing->isFileVideo(file))
            return Intermediate;
    }

    /* If it hasn't returned by now it should be fine. */
    return Acceptable;
}
