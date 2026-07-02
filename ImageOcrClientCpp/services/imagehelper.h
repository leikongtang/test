#ifndef IMAGEHELPER_H
#define IMAGEHELPER_H

#include <QString>

class ImageHelper
{
public:
    static QString fileToBase64(const QString &filePath, QString *errorMessage = nullptr);
};

#endif // IMAGEHELPER_H
