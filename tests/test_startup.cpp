#include <QtTest>
#include <QProcess>
#include <QProcessEnvironment>
#include <QTemporaryDir>

class StartupTest : public QObject
{
    Q_OBJECT
private:
    QString binary() const
    {
        return QCoreApplication::applicationDirPath() + "/boulderdash";
    }

private slots:
    void invalidArguments_data()
    {
        QTest::addColumn<QStringList>("arguments");
        for (const auto &value : {"0", "51", "-1", "abc", "1.5"})
            QTest::newRow(value) << (QStringList{"--level", value});
        QTest::newRow("missing") << QStringList{"--level"};
        QTest::newRow("duplicate") << QStringList{"--level", "1", "--level", "6"};
        QTest::newRow("gallery") << QStringList{"--level", "6", "--tile-gallery"};
    }

    void invalidArguments()
    {
        QFETCH(QStringList, arguments);
        QProcess process;
        process.start(binary(), arguments);
        QVERIFY(process.waitForFinished(5000));
        QCOMPARE(process.exitStatus(), QProcess::NormalExit);
        QVERIFY(process.exitCode() != 0);
        QVERIFY(process.readAllStandardError().contains("--level"));
    }

    void selectedLevel_data()
    {
        QTest::addColumn<int>("level");
        for (int level : {1, 6, 7, 50})
            QTest::newRow(qPrintable(QString::number(level))) << level;
    }

    void selectedLevel()
    {
        QFETCH(int, level);
        QTemporaryDir config;
        QVERIFY(config.isValid());
        QProcess process;
        auto env = QProcessEnvironment::systemEnvironment();
        env.insert("XDG_CONFIG_HOME", config.path());
        process.setProcessEnvironment(env);
        process.setProcessChannelMode(QProcess::MergedChannels);
        process.start(binary(), {"--level", QString::number(level)});
        QVERIFY(process.waitForStarted());
        const QByteArray expected = QString("Начальный уровень: %1 Авторских уровней: 6")
                                        .arg(level).toUtf8();
        QByteArray output;
        QElapsedTimer timer;
        timer.start();
        while (!output.contains(expected) && timer.elapsed() < 10000
               && process.state() != QProcess::NotRunning) {
            process.waitForReadyRead(100);
            output += process.readAll();
        }
        const bool started = output.contains(expected);
        process.terminate();
        if (!process.waitForFinished(2000)) {
            process.kill();
            process.waitForFinished();
        }
        QVERIFY2(started, output.constData());
        QVERIFY2(!output.contains("Неизвестный символ"), output.constData());
    }
};

QTEST_GUILESS_MAIN(StartupTest)
#include "test_startup.moc"
