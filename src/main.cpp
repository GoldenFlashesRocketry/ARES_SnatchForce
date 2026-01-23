// main.cpp

#define _USE_MATH_DEFINES

#include <QApplication>
#include <QWidget>
#include <QGridLayout>
#include <QGroupBox>
#include <QLineEdit>
#include <QLabel>
#include <QPushButton>
#include <QRadioButton>
#include <QCheckBox>
#include <QDoubleValidator>
#include <QComboBox>
#include <QMessageBox>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QScrollArea>

#include "snatchforce_core.h"

static void writeChuteReport(QTextStream& out,
                             const QString& title,
                             const ChuteResult& c)
{
    out << "--------------" << title << "--------------\n";
    out << "Mass (kg):              " << c.mass << "\n";
    out << "Deploy Altitude (m):    " << c.deployAlt << "\n";
    out << "Deploy Speed (m/s):     " << c.deploySpeed << "\n";
    out << "Air Density (kg/m^3):   " << c.rho << "\n";
    out << "q (Pa):                 " << c.q << "\n";
    out << "Chute Diameter (m):     " << c.chuteD << "\n";
    out << "Projected Area (m^2):   " << c.Sproj << "\n";
    out << "CoD:                    " << c.CoD << "\n";

    if (c.ckUsed.useExact)
    {
        out << "Ck:                     " << c.ckUsed.exact << "\n";
        out << "Estimated Fmax (N):     " << c.FmaxExact << "\n";
    }
    else
    {
        out << "Ck min/max/est:         "
            << c.ckUsed.ckMin << ", "
            << c.ckUsed.ckMax << ", "
            << c.ckUsed.ckEst << "\n";
        out << "Force (min) (N):               " << c.FmaxMin << "\n";
        out << "Force (max) (N):               " << c.FmaxMax << "\n";
        out << "Force (est) (N):               " << c.FmaxEst << "\n";
    }
    out << "\n\n";
}

struct ChuteControls
{
    QLineEdit* mass = nullptr;
    QLineEdit* alt = nullptr;
    QLineEdit* speed = nullptr;
    QLineEdit* cod = nullptr;
    QLineEdit* diameter = nullptr;

    QRadioButton* ckExactRadio = nullptr;
    QRadioButton* ckRangeRadio = nullptr;
    QLineEdit* ckExact = nullptr;
    QLineEdit* ckMin = nullptr;
    QLineEdit* ckMax = nullptr;
    QLineEdit* ckEst = nullptr;

    QCheckBox* autoVelocity = nullptr; // only used for main/payload
    QLabel* resultLabel = nullptr;
};

static QLineEdit* makeNumberEdit(QWidget* parent)
{
    auto* e = new QLineEdit(parent);
    e->setValidator(new QDoubleValidator(e));
    return e;
}

QGroupBox* createChuteGroup(const QString& title,
                            QWidget* parent,
                            bool includeAutoVelocity,
                            ChuteControls& out,
                            double defaultCkMin,
                            double defaultCkMax,
                            double defaultCkEst)
{
    auto* box = new QGroupBox(title, parent);
    auto* grid = new QGridLayout(box);
    int row = 0;

    grid->addWidget(new QLabel("Mass"), row, 0);
    out.mass = makeNumberEdit(box);
    grid->addWidget(out.mass, row++, 1);

    grid->addWidget(new QLabel("Deploy Alt"), row, 0);
    out.alt = makeNumberEdit(box);
    grid->addWidget(out.alt, row++, 1);

    if (includeAutoVelocity)
    {
        //grid->addWidget(new QLabel("Auto Speed"), row, 0);
        out.autoVelocity = new QCheckBox("Calc from terminal velocity", box);
        grid->addWidget(out.autoVelocity, row++, 1);
    }
    else
    {
        out.autoVelocity = nullptr;
        grid->addItem(new QSpacerItem(0, 30), row++, 0, 1, 2);

    }

    grid->addWidget(new QLabel("Deploy Speed"), row, 0);
    out.speed = makeNumberEdit(box);
    grid->addWidget(out.speed, row++, 1);

    grid->addWidget(new QLabel("CoD"), row, 0);
    out.cod = makeNumberEdit(box);
    grid->addWidget(out.cod, row++, 1);

    grid->addWidget(new QLabel("Diameter"), row, 0);
    out.diameter = makeNumberEdit(box);
    grid->addWidget(out.diameter, row++, 1);

    // Ck controls
    out.ckExactRadio = new QRadioButton("Exact Ck", box);
    out.ckRangeRadio = new QRadioButton("Range Ck", box);
    grid->addWidget(out.ckExactRadio, row, 0);
    grid->addWidget(out.ckRangeRadio, row++, 1);

    grid->addWidget(new QLabel("Ck exact"), row, 0);
    out.ckExact = makeNumberEdit(box);
    grid->addWidget(out.ckExact, row++, 1);

    grid->addWidget(new QLabel("Ck min"), row, 0);
    out.ckMin = makeNumberEdit(box);
    grid->addWidget(out.ckMin, row++, 1);

    grid->addWidget(new QLabel("Ck max"), row, 0);
    out.ckMax = makeNumberEdit(box);
    grid->addWidget(out.ckMax, row++, 1);

    grid->addWidget(new QLabel("Ck est"), row, 0);
    out.ckEst = makeNumberEdit(box);
    grid->addWidget(out.ckEst, row++, 1);

    // Defaults: use range by default, with preset min/max/est
    out.ckRangeRadio->setChecked(true);
    out.ckExactRadio->setChecked(false);
    out.ckMin->setText(QString::number(defaultCkMin));
    out.ckMax->setText(QString::number(defaultCkMax));
    out.ckEst->setText(QString::number(defaultCkEst));

    out.resultLabel = new QLabel("Result: -", box);
    out.resultLabel->setTextFormat(Qt::RichText);
    grid->addWidget(out.resultLabel, row++, 0, 1, 3);

    box->setLayout(grid);
    return box;
}

static double readDouble(QLineEdit* e)
{
    if (!e) return 0.0;
    const QString t = e->text().trimmed();
    if (t.isEmpty()) return 0.0;
    bool ok = false;
    const double v = t.toDouble(&ok);
    return ok ? v : 0.0;
}

static CkInput makeCkInput(const ChuteControls& c)
{
    CkInput ck{};
    ck.useExact = c.ckExactRadio->isChecked();
    if (ck.useExact)
    {
        ck.exact = readDouble(c.ckExact);
        ck.ckMin = ck.ckMax = ck.ckEst = 0.0;
    }
    else
    {
        ck.exact = 0.0;
        ck.ckMin = readDouble(c.ckMin);
        ck.ckMax = readDouble(c.ckMax);
        ck.ckEst = readDouble(c.ckEst);
    }
    return ck;
}

static QString formatResult(const ChuteResult& cr)
{
    QString s;
    s += QString("ρ = %1 kg/m³, q = %2 Pa<br>")
            .arg(cr.rho, 0, 'f', 4)
            .arg(cr.q, 0, 'f', 2);
    s += QString("Sproj = %1 m², Fsteady = %2 N<br>")
            .arg(cr.Sproj, 0, 'f', 4)
            .arg(cr.Fsteady, 0, 'f', 2);

    if (cr.ckUsed.useExact)
    {
        s += QString("Ck = %1 → F ≈ %2 N")
                .arg(cr.ckUsed.exact, 0, 'f', 3)
                .arg(cr.FmaxExact, 0, 'f', 2);
    }
    else
    {
        s += QString("Ck min/max/est = (%1,  %2,  %3)<br>")
                .arg(cr.ckUsed.ckMin, 0, 'f', 2)
                .arg(cr.ckUsed.ckMax, 0, 'f', 2)
                .arg(cr.ckUsed.ckEst, 0, 'f', 2);
        s += QString("Force (min) = %1 N <br>Force (max) = %2 N <br>Force (est) = %3 N")
                .arg(cr.FmaxMin, 0, 'f', 2)
                .arg(cr.FmaxMax, 0, 'f', 2)
                .arg(cr.FmaxEst, 0, 'f', 2);
    }
    return s;
}

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);

    QWidget window;
    window.setWindowTitle("Snatchforce Calculator");

    // Outer layout
    auto* outer = new QVBoxLayout(&window);

    // Scrollable central widget
    auto* central = new QWidget(&window);
    auto* layout = new QGridLayout(central);

    // Unit system selector
    auto* unitLabel = new QLabel("Units:", central);
    auto* unitCombo = new QComboBox(central);
    unitCombo->addItem("Metric (kg, m, m/s)");
    unitCombo->addItem("Imperial (lb, ft, mph)");
    layout->addWidget(unitLabel, 0, 0);
    layout->addWidget(unitCombo, 0, 1);

    ChuteControls drogueUI{};
    ChuteControls mainUI{};
    ChuteControls payloadUI{};

    ChuteResult drogueRes{};
    ChuteResult mainRes{};
    ChuteResult payloadRes{};
    bool haveResults = false;


    auto* drogueBox  = createChuteGroup("Drogue",  central, false, drogueUI,
                                        1.0, 1.4, 1.2);
    auto* mainBox    = createChuteGroup("Main",    central, true,  mainUI,
                                        1.0, 2.0, 1.5);
    auto* payloadBox = createChuteGroup("Payload", central, true,  payloadUI,
                                        1.0, 1.5, 1.25);

    layout->addWidget(drogueBox, 1, 0);
    layout->addWidget(mainBox,   1, 1);
    layout->addWidget(payloadBox,1, 2);
    

    // Buttons row
    auto* computeBtn = new QPushButton("Compute Snatchforces", central);
    auto* saveBtn    = new QPushButton("Save Report to File", central);
    layout->addWidget(computeBtn, 3, 0, 1, 3);
    layout->addWidget(saveBtn,    0, 2);

    central->setLayout(layout);

    // Put central inside a scroll area
    auto* scroll = new QScrollArea(&window);
    scroll->setWidget(central);
    scroll->setWidgetResizable(true);

    outer->addWidget(scroll);
    window.setLayout(outer);

    // Optionally set a reasonable initial size
    window.resize(900, 600);


    QObject::connect(computeBtn, &QPushButton::clicked, [&]() {
        try
        {
            const bool imperial = (unitCombo->currentIndex() == 1);

            // DROGUE input
            ChuteInput drogueIn{};
            drogueIn.imperial        = imperial;
            drogueIn.mass            = readDouble(drogueUI.mass);
            drogueIn.deployAlt       = readDouble(drogueUI.alt);
            drogueIn.deploySpeed     = readDouble(drogueUI.speed);
            drogueIn.CoD             = readDouble(drogueUI.cod);
            drogueIn.chuteD          = readDouble(drogueUI.diameter);
            drogueIn.useAutoVelocity = false;
            drogueIn.ck              = makeCkInput(drogueUI);

            drogueRes = computeChute(drogueIn, 0, nullptr);

            // MAIN input
            ChuteInput mainIn{};
            mainIn.imperial        = imperial;
            mainIn.mass            = readDouble(mainUI.mass);
            mainIn.deployAlt       = readDouble(mainUI.alt);
            mainIn.deploySpeed     = readDouble(mainUI.speed);
            mainIn.CoD             = readDouble(mainUI.cod);
            mainIn.chuteD          = readDouble(mainUI.diameter);
            mainIn.useAutoVelocity = (mainUI.autoVelocity &&
                                      mainUI.autoVelocity->isChecked());
            mainIn.ck              = makeCkInput(mainUI);

            mainRes = computeChute(mainIn,
                                   mainIn.mass,
                                   mainIn.useAutoVelocity ? &drogueRes : nullptr);

            // PAYLOAD input
            ChuteInput payloadIn{};
            payloadIn.imperial        = imperial;
            payloadIn.mass            = readDouble(payloadUI.mass);
            payloadIn.deployAlt       = readDouble(payloadUI.alt);
            payloadIn.deploySpeed     = readDouble(payloadUI.speed);
            payloadIn.CoD             = readDouble(payloadUI.cod);
            payloadIn.chuteD          = readDouble(payloadUI.diameter);
            payloadIn.useAutoVelocity = (payloadUI.autoVelocity &&
                                         payloadUI.autoVelocity->isChecked());
            payloadIn.ck              = makeCkInput(payloadUI);

            payloadRes = computeChute(payloadIn,
                                      mainIn.mass,
                                      payloadIn.useAutoVelocity ? &drogueRes : nullptr);

            drogueUI.resultLabel->setText(formatResult(drogueRes));
            mainUI.resultLabel->setText(formatResult(mainRes));
            payloadUI.resultLabel->setText(formatResult(payloadRes));

            haveResults = true;
        }
        catch (const std::exception& e)
        {
            QMessageBox::critical(&window, "Error", e.what());
        }
    });

    QObject::connect(saveBtn, &QPushButton::clicked, [&]() {
        if (!haveResults)
        {
            QMessageBox::warning(&window, "No data",
                                 "Compute snatchforces before saving a report.");
            return;
        }

        QString fileName = QFileDialog::getSaveFileName(
            &window,
            "Save Snatchforce Report",
            "SnatchforceReport.txt",
            "Text Files (*.txt);;All Files (*.*)");

        if (fileName.isEmpty())
            return;

        QFile file(fileName);
        if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
            QMessageBox::critical(&window, "Error",
                                  "Could not open file for writing.");
            return;
        }

        QTextStream out(&file);
        out << "Snatchforce Report (all values in SI units)\n\n";
        writeChuteReport(out, "Drogue Chute",  drogueRes);
        writeChuteReport(out, "Main Chute",    mainRes);
        writeChuteReport(out, "Payload Chute", payloadRes);
    });


    window.setLayout(layout);
    window.show();
    return app.exec();
}
