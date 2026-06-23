#pragma once

#include <QDateTime>
#include <QString>
#include <QStringList>
#include <QVector>

namespace CommonTools {

struct ReferenceRow {
    QString code;
    QString label;
    QString note;
};

struct RegexGroup {
    int index = 0;
    QString name;
    QString text;
    bool matched = false;
};

struct RegexMatch {
    QString text;
    int start = 0;
    int length = 0;
    QVector<RegexGroup> groups;
};

struct RegexOptions {
    bool caseInsensitive = false;
    bool multiline = false;
    bool dotMatchesAll = false;
};

QString formatJson(const QString &input, QString *error);
QString textToBase64(const QString &text);
QString base64ToText(const QString &base64, QString *error);
QString hexToString(const QString &hex, QString *error);
QString timestampToLocalText(const QString &timestamp, QString *error);
QString compareLines(const QString &left, const QString &right);

struct LineDiff {
    QVector<int> leftChangedLines;
    QVector<int> rightChangedLines;
    int sameCount = 0;
    int diffCount = 0;
};

LineDiff diffLineIndices(const QString &left, const QString &right);

struct CronSchedule {
    QString expression;
    QString description;
    QVector<QDateTime> nextRuns;
    bool valid = false;
};

CronSchedule analyzeCron(const QString &expression, const QDateTime &from, int count, QString *error);

QStringList generateUuids(int count, bool uppercase, bool withoutDashes);

struct HashResult {
    QString md5;
    QString sha1;
    QString sha256;
    QString sha512;
};

HashResult computeHashes(const QByteArray &data);

QString urlEncode(const QString &text);
QString urlDecode(const QString &text);

QString decodeJwt(const QString &token, QString *error);

struct NumberBases {
    QString binary;
    QString octal;
    QString decimal;
    QString hex;
    bool valid = false;
};

NumberBases convertNumberBase(const QString &input, int fromBase, QString *error);

struct CaseForms {
    QString camel;
    QString pascal;
    QString snake;
    QString kebab;
    QString constantCase;
    QString title;
};

CaseForms convertCase(const QString &text);
QString matchRegularExpression(const QString &pattern, const QString &text, QString *error);
QVector<RegexMatch> runRegularExpression(const QString &pattern,
                                         const QString &text,
                                         const RegexOptions &options,
                                         QString *error);
QString maskSensitiveText(const QString &text);
QString mockFromJsonExample(const QString &json, QString *error);
QString describeCron(const QString &expression, QString *error);

QString htmlEncode(const QString &text);
QString htmlDecode(const QString &text);

qint64 currentTimestampMs();
QString timestampToDateText(qint64 value, bool milliseconds, const QString &format);
qint64 dateTextToTimestampMs(const QString &text, const QString &format, QString *error);

QVector<ReferenceRow> httpStatusRows();
QVector<ReferenceRow> htmlEntityRows();

}
