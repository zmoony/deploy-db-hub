#include "tools/CommonTools.h"

#include <algorithm>

#include <QByteArray>
#include <QCryptographicHash>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QJsonParseError>
#include <QLocale>
#include <QPair>
#include <QRegularExpression>
#include <QSet>
#include <QStringList>
#include <QUrl>
#include <QUuid>

namespace {

void setError(QString *error, const QString &message)
{
    if (error != nullptr) {
        *error = message;
    }
}

void clearError(QString *error)
{
    if (error != nullptr) {
        error->clear();
    }
}

QJsonValue mockValue(const QString &key, const QJsonValue &value, int index)
{
    if (value.isString()) {
        return QStringLiteral("%1_mock_%2").arg(key.isEmpty() ? QStringLiteral("value") : key).arg(index);
    }
    if (value.isDouble()) {
        return 100 + index;
    }
    if (value.isBool()) {
        return !value.toBool();
    }
    if (value.isArray()) {
        QJsonArray array;
        const QJsonArray source = value.toArray();
        if (source.isEmpty()) {
            array.append(QStringLiteral("item_mock_%1").arg(index));
        } else {
            array.append(mockValue(key, source.first(), index));
        }
        return array;
    }
    if (value.isObject()) {
        QJsonObject object;
        const QJsonObject source = value.toObject();
        for (auto it = source.begin(); it != source.end(); ++it) {
            object.insert(it.key(), mockValue(it.key(), it.value(), index));
        }
        return object;
    }
    return QStringLiteral("%1_mock_%2").arg(key.isEmpty() ? QStringLiteral("value") : key).arg(index);
}

}

namespace CommonTools {

QString formatJson(const QString &input, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(input.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setError(error, parseError.errorString());
        return {};
    }
    clearError(error);
    return QString::fromUtf8(document.toJson(QJsonDocument::Indented)).trimmed();
}

QString textToBase64(const QString &text)
{
    return QString::fromLatin1(text.toUtf8().toBase64());
}

QString base64ToText(const QString &base64, QString *error)
{
    const QByteArray input = base64.trimmed().toLatin1();
    const QByteArray decoded = QByteArray::fromBase64(input, QByteArray::Base64Option::AbortOnBase64DecodingErrors);
    if (decoded.isEmpty() && !input.isEmpty()) {
        setError(error, QStringLiteral("Base64 内容无效"));
        return {};
    }
    clearError(error);
    return QString::fromUtf8(decoded);
}

QString hexToString(const QString &hex, QString *error)
{
    QString normalized = hex;
    normalized.remove(QLatin1Char(' '));
    normalized.remove(QLatin1Char('\n'));
    normalized.remove(QLatin1Char('\r'));
    normalized.remove(QLatin1Char('\t'));
    normalized.remove(QStringLiteral("0x"), Qt::CaseInsensitive);
    if (normalized.size() % 2 != 0) {
        setError(error, QStringLiteral("Hex 长度必须为偶数"));
        return {};
    }

    const QByteArray decoded = QByteArray::fromHex(normalized.toLatin1());
    if (decoded.toHex().compare(normalized.toLatin1(), Qt::CaseInsensitive) != 0) {
        setError(error, QStringLiteral("Hex 内容无效"));
        return {};
    }
    clearError(error);
    return QString::fromUtf8(decoded);
}

QString timestampToLocalText(const QString &timestamp, QString *error)
{
    bool ok = false;
    const qint64 raw = timestamp.trimmed().toLongLong(&ok);
    if (!ok) {
        setError(error, QStringLiteral("时间戳必须为数字"));
        return {};
    }
    const qint64 seconds = timestamp.trimmed().size() >= 13 ? raw / 1000 : raw;
    const QDateTime time = QDateTime::fromSecsSinceEpoch(seconds).toLocalTime();
    clearError(error);
    return QLocale::system().toString(time, QStringLiteral("yyyy-MM-dd HH:mm:ss"));
}

QString compareLines(const QString &left, const QString &right)
{
    const QStringList leftLines = left.split(QLatin1Char('\n'));
    const QStringList rightLines = right.split(QLatin1Char('\n'));
    QStringList output;
    const int maxLines = qMax(leftLines.size(), rightLines.size());
    for (int i = 0; i < maxLines; ++i) {
        const QString leftLine = i < leftLines.size() ? leftLines.at(i) : QString();
        const QString rightLine = i < rightLines.size() ? rightLines.at(i) : QString();
        if (leftLine == rightLine) {
            output.append(QStringLiteral("  %1").arg(leftLine));
        } else {
            if (i < leftLines.size()) {
                output.append(QStringLiteral("- %1").arg(leftLine));
            }
            if (i < rightLines.size()) {
                output.append(QStringLiteral("+ %1").arg(rightLine));
            }
        }
    }
    return output.join(QLatin1Char('\n'));
}

LineDiff diffLineIndices(const QString &left, const QString &right)
{
    LineDiff result;
    const QStringList a = left.split(QLatin1Char('\n'));
    const QStringList b = right.split(QLatin1Char('\n'));
    const int n = a.size();
    const int m = b.size();

    if (static_cast<qint64>(n) * static_cast<qint64>(m) > 4000000) {
        const int maxLines = qMax(n, m);
        for (int i = 0; i < maxLines; ++i) {
            if (i >= n) {
                result.rightChangedLines.append(i);
                ++result.diffCount;
            } else if (i >= m) {
                result.leftChangedLines.append(i);
                ++result.diffCount;
            } else if (a.at(i) == b.at(i)) {
                ++result.sameCount;
            } else {
                result.leftChangedLines.append(i);
                result.rightChangedLines.append(i);
                ++result.diffCount;
            }
        }
        return result;
    }

    QVector<QVector<int>> dp(n + 1, QVector<int>(m + 1, 0));
    for (int i = n - 1; i >= 0; --i) {
        for (int j = m - 1; j >= 0; --j) {
            dp[i][j] = (a.at(i) == b.at(j))
                ? dp[i + 1][j + 1] + 1
                : qMax(dp[i + 1][j], dp[i][j + 1]);
        }
    }

    QVector<bool> leftMatched(n, false);
    QVector<bool> rightMatched(m, false);
    int i = 0;
    int j = 0;
    while (i < n && j < m) {
        if (a.at(i) == b.at(j)) {
            leftMatched[i] = true;
            rightMatched[j] = true;
            ++i;
            ++j;
        } else if (dp[i + 1][j] >= dp[i][j + 1]) {
            ++i;
        } else {
            ++j;
        }
    }

    for (int k = 0; k < n; ++k) {
        if (leftMatched.at(k)) {
            ++result.sameCount;
        } else {
            result.leftChangedLines.append(k);
        }
    }
    for (int k = 0; k < m; ++k) {
        if (!rightMatched.at(k)) {
            result.rightChangedLines.append(k);
        }
    }
    result.diffCount = result.leftChangedLines.size() + result.rightChangedLines.size();
    return result;
}

QString matchRegularExpression(const QString &pattern, const QString &text, QString *error)
{
    const QRegularExpression expression(pattern);
    if (!expression.isValid()) {
        setError(error, expression.errorString());
        return {};
    }

    QStringList output;
    QRegularExpressionMatchIterator it = expression.globalMatch(text);
    int index = 1;
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        output.append(QStringLiteral("匹配 %1: %2").arg(index++).arg(match.captured(0)));
        for (int group = 1; group <= match.lastCapturedIndex(); ++group) {
            output.append(QStringLiteral("  组 %1: %2").arg(group).arg(match.captured(group)));
        }
    }
    clearError(error);
    return output.isEmpty() ? QStringLiteral("未匹配") : output.join(QLatin1Char('\n'));
}

QVector<RegexMatch> runRegularExpression(const QString &pattern,
                                         const QString &text,
                                         const RegexOptions &options,
                                         QString *error)
{
    QVector<RegexMatch> results;
    QRegularExpression::PatternOptions patternOptions = QRegularExpression::NoPatternOption;
    if (options.caseInsensitive) {
        patternOptions |= QRegularExpression::CaseInsensitiveOption;
    }
    if (options.multiline) {
        patternOptions |= QRegularExpression::MultilineOption;
    }
    if (options.dotMatchesAll) {
        patternOptions |= QRegularExpression::DotMatchesEverythingOption;
    }

    QRegularExpression expression(pattern, patternOptions);
    if (!expression.isValid()) {
        setError(error, expression.errorString());
        return results;
    }

    const QStringList namedGroups = expression.namedCaptureGroups();
    QRegularExpressionMatchIterator it = expression.globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch match = it.next();
        RegexMatch entry;
        entry.text = match.captured(0);
        entry.start = match.capturedStart(0);
        entry.length = match.capturedLength(0);
        for (int group = 1; group <= match.lastCapturedIndex(); ++group) {
            RegexGroup info;
            info.index = group;
            info.name = group < namedGroups.size() ? namedGroups.at(group) : QString();
            info.matched = match.capturedStart(group) >= 0;
            info.text = match.captured(group);
            entry.groups.append(info);
        }
        results.append(entry);
    }
    clearError(error);
    return results;
}

QString maskSensitiveText(const QString &text)
{
    QString masked = text;
    masked.replace(QRegularExpression(QStringLiteral("\\b(1[3-9]\\d)\\d{4}(\\d{4})\\b")),
                   QStringLiteral("\\1****\\2"));
    masked.replace(QRegularExpression(QStringLiteral("\\b([A-Za-z0-9._%+-])([A-Za-z0-9._%+-]*)(@[A-Za-z0-9.-]+\\.[A-Za-z]{2,})\\b")),
                   QStringLiteral("\\1***\\3"));
    masked.replace(QRegularExpression(QStringLiteral("\\b(\\d{6})\\d{8}(\\w{4})\\b")),
                   QStringLiteral("\\1********\\2"));
    return masked;
}

QString mockFromJsonExample(const QString &json, QString *error)
{
    QJsonParseError parseError;
    const QJsonDocument document = QJsonDocument::fromJson(json.toUtf8(), &parseError);
    if (parseError.error != QJsonParseError::NoError) {
        setError(error, parseError.errorString());
        return {};
    }

    QJsonDocument output;
    if (document.isObject()) {
        output = QJsonDocument(mockValue(QString(), document.object(), 1).toObject());
    } else if (document.isArray()) {
        QJsonArray array;
        const QJsonArray source = document.array();
        if (!source.isEmpty()) {
            array.append(mockValue(QString(), source.first(), 1));
            array.append(mockValue(QString(), source.first(), 2));
        }
        output = QJsonDocument(array);
    }
    clearError(error);
    return QString::fromUtf8(output.toJson(QJsonDocument::Indented)).trimmed();
}

QString describeCron(const QString &expression, QString *error)
{
    const QStringList parts = expression.simplified().split(QLatin1Char(' '));
    if (parts.size() != 5) {
        setError(error, QStringLiteral("Cron 表达式需要 5 段"));
        return {};
    }
    clearError(error);
    if (parts.at(0).startsWith(QStringLiteral("*/"))) {
        return QStringLiteral("每 %1 分钟执行一次").arg(parts.at(0).mid(2));
    }
    if (parts.at(0) == QStringLiteral("0") && parts.at(1).startsWith(QStringLiteral("*/"))) {
        return QStringLiteral("每 %1 小时执行一次").arg(parts.at(1).mid(2));
    }
    return QStringLiteral("分钟=%1，小时=%2，日=%3，月=%4，周=%5")
        .arg(parts.at(0), parts.at(1), parts.at(2), parts.at(3), parts.at(4));
}

namespace {

struct CronField {
    QSet<int> values;
    bool wildcard = false;
};

bool parseCronField(const QString &raw, int lo, int hi, bool dayOfWeek, CronField *out, QString *error)
{
    const QString field = raw.trimmed();
    if (field.isEmpty()) {
        setError(error, QStringLiteral("Cron 字段不能为空"));
        return false;
    }
    if (field == QStringLiteral("*") || field == QStringLiteral("?")) {
        out->wildcard = true;
        for (int v = lo; v <= hi; ++v) {
            out->values.insert(v);
        }
        return true;
    }

    const QStringList tokens = field.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &token : tokens) {
        QString base = token;
        int step = 1;
        const int slash = token.indexOf(QLatin1Char('/'));
        if (slash >= 0) {
            base = token.left(slash);
            bool ok = false;
            step = token.mid(slash + 1).toInt(&ok);
            if (!ok || step <= 0) {
                setError(error, QStringLiteral("Cron 步长无效：%1").arg(token));
                return false;
            }
        }

        int start = lo;
        int end = hi;
        if (base == QStringLiteral("*") || base.isEmpty()) {
            start = lo;
            end = hi;
        } else if (base.contains(QLatin1Char('-'))) {
            const QStringList range = base.split(QLatin1Char('-'));
            if (range.size() != 2) {
                setError(error, QStringLiteral("Cron 范围无效：%1").arg(token));
                return false;
            }
            bool okA = false;
            bool okB = false;
            start = range.at(0).toInt(&okA);
            end = range.at(1).toInt(&okB);
            if (!okA || !okB) {
                setError(error, QStringLiteral("Cron 范围无效：%1").arg(token));
                return false;
            }
        } else {
            bool ok = false;
            start = base.toInt(&ok);
            if (!ok) {
                setError(error, QStringLiteral("Cron 字段无效：%1").arg(token));
                return false;
            }
            end = (slash >= 0) ? hi : start;
        }

        for (int v = start; v <= end; v += step) {
            int value = v;
            if (dayOfWeek && value == 7) {
                value = 0;
            }
            if (value < lo || value > hi) {
                setError(error, QStringLiteral("Cron 值越界：%1").arg(token));
                return false;
            }
            out->values.insert(value);
        }
    }

    if (out->values.isEmpty()) {
        setError(error, QStringLiteral("Cron 字段无有效值：%1").arg(field));
        return false;
    }
    return true;
}

QString describeCronField(const QString &name, const CronField &field, int lo, int hi)
{
    if (field.wildcard) {
        return QStringLiteral("每%1").arg(name);
    }
    QList<int> values = field.values.values();
    std::sort(values.begin(), values.end());
    if (values.size() == (hi - lo + 1)) {
        return QStringLiteral("每%1").arg(name);
    }
    QStringList parts;
    for (int v : values) {
        parts.append(QString::number(v));
    }
    return QStringLiteral("%1 = %2").arg(name, parts.join(QLatin1Char(',')));
}

int cronDayOfWeek(const QDate &date)
{
    return date.dayOfWeek() % 7; // Qt: Mon=1..Sun=7 -> cron Sun=0..Sat=6
}

}

CronSchedule analyzeCron(const QString &expression, const QDateTime &from, int count, QString *error)
{
    CronSchedule schedule;
    schedule.expression = expression.simplified();

    const QStringList fields = schedule.expression.split(QLatin1Char(' '), Qt::SkipEmptyParts);
    if (fields.size() < 5 || fields.size() > 7) {
        setError(error, QStringLiteral("Cron 需要 5~7 段（秒 分 时 日 月 周 年）"));
        return schedule;
    }

    QString secStr = QStringLiteral("0");
    QString minStr;
    QString hourStr;
    QString domStr;
    QString monStr;
    QString dowStr;
    QString yearStr = QStringLiteral("*");

    if (fields.size() == 5) {
        minStr = fields.at(0);
        hourStr = fields.at(1);
        domStr = fields.at(2);
        monStr = fields.at(3);
        dowStr = fields.at(4);
    } else if (fields.size() == 6) {
        secStr = fields.at(0);
        minStr = fields.at(1);
        hourStr = fields.at(2);
        domStr = fields.at(3);
        monStr = fields.at(4);
        dowStr = fields.at(5);
    } else {
        secStr = fields.at(0);
        minStr = fields.at(1);
        hourStr = fields.at(2);
        domStr = fields.at(3);
        monStr = fields.at(4);
        dowStr = fields.at(5);
        yearStr = fields.at(6);
    }

    CronField sec;
    CronField minute;
    CronField hour;
    CronField dom;
    CronField month;
    CronField dow;
    CronField year;
    if (!parseCronField(secStr, 0, 59, false, &sec, error)
        || !parseCronField(minStr, 0, 59, false, &minute, error)
        || !parseCronField(hourStr, 0, 23, false, &hour, error)
        || !parseCronField(domStr, 1, 31, false, &dom, error)
        || !parseCronField(monStr, 1, 12, false, &month, error)
        || !parseCronField(dowStr, 0, 6, true, &dow, error)
        || !parseCronField(yearStr, 1970, 2099, false, &year, error)) {
        return schedule;
    }
    const bool domWild = (domStr.trimmed() == QStringLiteral("*") || domStr.trimmed() == QStringLiteral("?"));
    const bool dowWild = (dowStr.trimmed() == QStringLiteral("*") || dowStr.trimmed() == QStringLiteral("?"));

    schedule.valid = true;
    QStringList parts;
    parts.append(describeCronField(QStringLiteral("秒"), sec, 0, 59));
    parts.append(describeCronField(QStringLiteral("分"), minute, 0, 59));
    parts.append(describeCronField(QStringLiteral("时"), hour, 0, 23));
    parts.append(describeCronField(QStringLiteral("日"), dom, 1, 31));
    parts.append(describeCronField(QStringLiteral("月"), month, 1, 12));
    parts.append(describeCronField(QStringLiteral("周"), dow, 0, 6));
    if (fields.size() == 7) {
        parts.append(describeCronField(QStringLiteral("年"), year, 1970, 2099));
    }
    schedule.description = parts.join(QStringLiteral("，"));

    const int wanted = count > 0 ? count : 5;
    QDateTime cursor = from;
    cursor = cursor.addMSecs(-cursor.time().msec());
    cursor = cursor.addSecs(1);

    int guard = 0;
    const int maxGuard = 2000000;
    while (schedule.nextRuns.size() < wanted && guard < maxGuard) {
        ++guard;
        const QDate date = cursor.date();
        const QTime time = cursor.time();

        if (!year.wildcard && !year.values.contains(date.year())) {
            if (date.year() > 2099) {
                break;
            }
            cursor = QDateTime(QDate(date.year() + 1, 1, 1), QTime(0, 0, 0));
            continue;
        }
        if (!month.values.contains(date.month())) {
            int ny = date.year();
            int nm = date.month() + 1;
            if (nm > 12) {
                nm = 1;
                ++ny;
            }
            cursor = QDateTime(QDate(ny, nm, 1), QTime(0, 0, 0));
            continue;
        }

        bool dayOk = false;
        if (domWild && dowWild) {
            dayOk = true;
        } else if (domWild) {
            dayOk = dow.values.contains(cronDayOfWeek(date));
        } else if (dowWild) {
            dayOk = dom.values.contains(date.day());
        } else {
            dayOk = dom.values.contains(date.day()) || dow.values.contains(cronDayOfWeek(date));
        }
        if (!dayOk) {
            cursor = QDateTime(date.addDays(1), QTime(0, 0, 0));
            continue;
        }
        if (!hour.values.contains(time.hour())) {
            cursor = QDateTime(date, QTime(time.hour(), 0, 0)).addSecs(3600);
            continue;
        }
        if (!minute.values.contains(time.minute())) {
            cursor = QDateTime(date, QTime(time.hour(), time.minute(), 0)).addSecs(60);
            continue;
        }
        if (!sec.values.contains(time.second())) {
            cursor = cursor.addSecs(1);
            continue;
        }

        schedule.nextRuns.append(cursor);
        cursor = cursor.addSecs(1);
    }

    clearError(error);
    return schedule;
}

QString htmlEncode(const QString &text)
{
    QString out;
    out.reserve(text.size());
    for (const QChar ch : text) {
        switch (ch.unicode()) {
        case u'&': out += QStringLiteral("&amp;"); break;
        case u'<': out += QStringLiteral("&lt;"); break;
        case u'>': out += QStringLiteral("&gt;"); break;
        case u'"': out += QStringLiteral("&quot;"); break;
        case u'\'': out += QStringLiteral("&#39;"); break;
        default: out += ch; break;
        }
    }
    return out;
}

QString htmlDecode(const QString &text)
{
    QString out = text;

    static const QVector<QPair<QString, QString>> namedEntities = {
        {QStringLiteral("&lt;"), QStringLiteral("<")},
        {QStringLiteral("&gt;"), QStringLiteral(">")},
        {QStringLiteral("&quot;"), QStringLiteral("\"")},
        {QStringLiteral("&#39;"), QStringLiteral("'")},
        {QStringLiteral("&apos;"), QStringLiteral("'")},
        {QStringLiteral("&nbsp;"), QStringLiteral(" ")},
        {QStringLiteral("&copy;"), QString::fromUtf8("\xC2\xA9")},
        {QStringLiteral("&reg;"), QString::fromUtf8("\xC2\xAE")},
        {QStringLiteral("&trade;"), QString::fromUtf8("\xE2\x84\xA2")}
    };
    for (const auto &entity : namedEntities) {
        out.replace(entity.first, entity.second);
    }

    static const QRegularExpression decimalEntity(QStringLiteral("&#(\\d+);"));
    static const QRegularExpression hexEntity(QStringLiteral("&#x([0-9a-fA-F]+);"));
    for (int pass = 0; pass < 2; ++pass) {
        const bool hex = (pass == 1);
        const QRegularExpression &pattern = hex ? hexEntity : decimalEntity;
        QString rebuilt;
        int last = 0;
        QRegularExpressionMatchIterator it = pattern.globalMatch(out);
        while (it.hasNext()) {
            const QRegularExpressionMatch match = it.next();
            rebuilt += out.mid(last, match.capturedStart(0) - last);
            bool ok = false;
            const uint code = match.captured(1).toUInt(&ok, hex ? 16 : 10);
            if (ok && code != 0) {
                rebuilt += QString(QChar(code));
            } else {
                rebuilt += match.captured(0);
            }
            last = match.capturedEnd(0);
        }
        rebuilt += out.mid(last);
        out = rebuilt;
    }

    out.replace(QStringLiteral("&amp;"), QStringLiteral("&"));
    return out;
}

qint64 currentTimestampMs()
{
    return QDateTime::currentMSecsSinceEpoch();
}

QString timestampToDateText(qint64 value, bool milliseconds, const QString &format)
{
    const qint64 ms = milliseconds ? value : value * 1000;
    const QDateTime time = QDateTime::fromMSecsSinceEpoch(ms).toLocalTime();
    const QString pattern = format.isEmpty() ? QStringLiteral("yyyy-MM-dd HH:mm:ss") : format;
    return time.toString(pattern);
}

qint64 dateTextToTimestampMs(const QString &text, const QString &format, QString *error)
{
    const QString trimmed = text.trimmed();
    const QString pattern = format.isEmpty() ? QStringLiteral("yyyy-MM-dd HH:mm:ss") : format;
    QDateTime time = QDateTime::fromString(trimmed, pattern);
    if (!time.isValid()) {
        time = QDateTime::fromString(trimmed, Qt::ISODate);
    }
    if (!time.isValid()) {
        setError(error, QStringLiteral("无法按格式解析时间"));
        return -1;
    }
    if (time.timeSpec() == Qt::TimeZone || time.timeSpec() == Qt::OffsetFromUTC) {
        // keep the parsed zone
    } else {
        time.setTimeSpec(Qt::LocalTime);
    }
    clearError(error);
    return time.toMSecsSinceEpoch();
}

QStringList generateUuids(int count, bool uppercase, bool withoutDashes)
{
    QStringList result;
    const int total = qBound(1, count, 1000);
    for (int i = 0; i < total; ++i) {
        QString value = QUuid::createUuid().toString(QUuid::WithoutBraces);
        if (withoutDashes) {
            value.remove(QLatin1Char('-'));
        }
        value = uppercase ? value.toUpper() : value.toLower();
        result.append(value);
    }
    return result;
}

HashResult computeHashes(const QByteArray &data)
{
    HashResult result;
    result.md5 = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
    result.sha1 = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha1).toHex());
    result.sha256 = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha256).toHex());
    result.sha512 = QString::fromLatin1(QCryptographicHash::hash(data, QCryptographicHash::Sha512).toHex());
    return result;
}

QString urlEncode(const QString &text)
{
    return QString::fromLatin1(QUrl::toPercentEncoding(text));
}

QString urlDecode(const QString &text)
{
    return QUrl::fromPercentEncoding(text.toUtf8());
}

QString decodeJwt(const QString &token, QString *error)
{
    const QStringList parts = token.trimmed().split(QLatin1Char('.'));
    if (parts.size() < 2) {
        setError(error, QStringLiteral("JWT 至少包含 header.payload 两段"));
        return {};
    }

    auto decodeSegment = [](const QString &segment) -> QString {
        QByteArray normalized = segment.toLatin1();
        normalized.replace('-', '+');
        normalized.replace('_', '/');
        while (normalized.size() % 4 != 0) {
            normalized.append('=');
        }
        const QByteArray raw = QByteArray::fromBase64(normalized);
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(raw, &parseError);
        if (parseError.error == QJsonParseError::NoError) {
            return QString::fromUtf8(document.toJson(QJsonDocument::Indented)).trimmed();
        }
        return QString::fromUtf8(raw);
    };

    QStringList output;
    output.append(QStringLiteral("Header:"));
    output.append(decodeSegment(parts.at(0)));
    output.append(QString());
    output.append(QStringLiteral("Payload:"));
    output.append(decodeSegment(parts.at(1)));
    if (parts.size() >= 3) {
        output.append(QString());
        output.append(QStringLiteral("Signature (raw):"));
        output.append(parts.at(2));
    }
    clearError(error);
    return output.join(QLatin1Char('\n'));
}

NumberBases convertNumberBase(const QString &input, int fromBase, QString *error)
{
    NumberBases bases;
    const QString trimmed = input.trimmed();
    if (trimmed.isEmpty()) {
        setError(error, QStringLiteral("请输入数字"));
        return bases;
    }
    bool ok = false;
    const qlonglong value = trimmed.toLongLong(&ok, fromBase);
    if (!ok) {
        setError(error, QStringLiteral("无法按 %1 进制解析：%2").arg(fromBase).arg(trimmed));
        return bases;
    }
    bases.binary = QString::number(value, 2);
    bases.octal = QString::number(value, 8);
    bases.decimal = QString::number(value, 10);
    bases.hex = QString::number(value, 16).toUpper();
    bases.valid = true;
    clearError(error);
    return bases;
}

CaseForms convertCase(const QString &text)
{
    CaseForms forms;
    QStringList words;
    QString current;
    auto flush = [&words, &current]() {
        if (!current.isEmpty()) {
            words.append(current.toLower());
            current.clear();
        }
    };
    for (int i = 0; i < text.size(); ++i) {
        const QChar ch = text.at(i);
        if (ch.isLetterOrNumber()) {
            const bool boundary = ch.isUpper() && !current.isEmpty()
                && (i > 0 && !text.at(i - 1).isUpper());
            if (boundary) {
                flush();
            }
            current.append(ch);
        } else {
            flush();
        }
    }
    flush();

    if (words.isEmpty()) {
        return forms;
    }

    QStringList capitalized;
    for (const QString &word : words) {
        QString cap = word;
        if (!cap.isEmpty()) {
            cap[0] = cap.at(0).toUpper();
        }
        capitalized.append(cap);
    }

    forms.snake = words.join(QLatin1Char('_'));
    forms.kebab = words.join(QLatin1Char('-'));
    forms.constantCase = forms.snake.toUpper();
    forms.pascal = capitalized.join(QString());
    forms.title = capitalized.join(QLatin1Char(' '));
    forms.camel = forms.pascal;
    if (!forms.camel.isEmpty()) {
        forms.camel[0] = forms.camel.at(0).toLower();
    }
    return forms;
}

QVector<ReferenceRow> httpStatusRows()
{
    return {
        {QStringLiteral("200"), QStringLiteral("OK"), QStringLiteral("请求成功")},
        {QStringLiteral("201"), QStringLiteral("Created"), QStringLiteral("资源已创建")},
        {QStringLiteral("204"), QStringLiteral("No Content"), QStringLiteral("成功且无响应体")},
        {QStringLiteral("301"), QStringLiteral("Moved Permanently"), QStringLiteral("永久重定向")},
        {QStringLiteral("302"), QStringLiteral("Found"), QStringLiteral("临时重定向")},
        {QStringLiteral("400"), QStringLiteral("Bad Request"), QStringLiteral("请求格式错误")},
        {QStringLiteral("401"), QStringLiteral("Unauthorized"), QStringLiteral("未认证")},
        {QStringLiteral("403"), QStringLiteral("Forbidden"), QStringLiteral("无权限")},
        {QStringLiteral("404"), QStringLiteral("Not Found"), QStringLiteral("资源不存在")},
        {QStringLiteral("409"), QStringLiteral("Conflict"), QStringLiteral("资源冲突")},
        {QStringLiteral("429"), QStringLiteral("Too Many Requests"), QStringLiteral("请求过于频繁")},
        {QStringLiteral("500"), QStringLiteral("Internal Server Error"), QStringLiteral("服务端错误")},
        {QStringLiteral("502"), QStringLiteral("Bad Gateway"), QStringLiteral("网关错误")},
        {QStringLiteral("503"), QStringLiteral("Service Unavailable"), QStringLiteral("服务不可用")}
    };
}

QVector<ReferenceRow> htmlEntityRows()
{
    return {
        {QStringLiteral("&quot;"), QStringLiteral("\""), QStringLiteral("双引号")},
        {QStringLiteral("&amp;"), QStringLiteral("&"), QStringLiteral("与号")},
        {QStringLiteral("&lt;"), QStringLiteral("<"), QStringLiteral("小于号")},
        {QStringLiteral("&gt;"), QStringLiteral(">"), QStringLiteral("大于号")},
        {QStringLiteral("&nbsp;"), QStringLiteral(" "), QStringLiteral("不换行空格")},
        {QStringLiteral("&copy;"), QString::fromUtf8("\xC2\xA9"), QStringLiteral("版权符号")},
        {QStringLiteral("&reg;"), QString::fromUtf8("\xC2\xAE"), QStringLiteral("注册商标")},
        {QStringLiteral("&trade;"), QString::fromUtf8("\xE2\x84\xA2"), QStringLiteral("商标")},
        {QStringLiteral("&apos;"), QStringLiteral("'"), QStringLiteral("单引号")}
    };
}

}
