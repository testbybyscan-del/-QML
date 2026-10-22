#ifndef LANDSCAPEGENERATOR_H
#define LANDSCAPEGENERATOR_H

#include <QString>
#include <QVector>
#include <QtGlobal>

// Создаёт карту отдельно от игрового движка. Такое разделение позволяет
// проверять правила генерации и читать алгоритм, не смешивая его с движением.
class LandscapeGenerator
{
public:
    // Одинаковый seed и та же версия алгоритма дают одинаковую карту.
    // При ошибке возвращается пустой вектор, а причина записывается в error.
    static QVector<QString> generate(quint32 seed, QString *error = nullptr);
};

#endif // LANDSCAPEGENERATOR_H
