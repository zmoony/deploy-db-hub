#pragma once

#include <QString>

enum class ServiceProductKind {
    Kafka,
    Redis,
    Elasticsearch,
    Oracle,
    PostgreSQL
};

QString serviceProductKindKey(ServiceProductKind kind);
QString serviceProductKindLabel(ServiceProductKind kind);
QString serviceParentCategoryLabel(ServiceProductKind kind);
QString serviceNodeInfoPlaceholder(ServiceProductKind kind);
QString serviceNodeFillInstructions(ServiceProductKind kind);
