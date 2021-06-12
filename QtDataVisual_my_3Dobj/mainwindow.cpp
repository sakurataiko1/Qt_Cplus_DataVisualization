#include "mainwindow.h"
#include "ui_mainwindow.h"

//参考：
//・3Dグラフ表示　座標ファイルを読み込んで散布図(scatterとして表示)　（マウス操作：ズームあり、視点移動なし）
//https://github.com/fengfanchen/Qt/tree/master/3D%20move%20rotation
//
//・参考：Qt付属のExampleコード　customplot(CSVファイルデータ入力でグラフ表示), bars　（マウス操作：ズームあり、視点移動あり）

#include <QtDataVisualization/QScatter3DSeries>
#include <QDebug>
#include <QThread>
#include <QMessageBox>
#include <QHBoxLayout>

//-start- Qt_Official_sample:custominput(CSVファイルデータ入力でグラフ表示) , bars(視点移動)
#include <QtDataVisualization/QScatterDataProxy>
#include <QtDataVisualization/QValue3DAxis>
#include <QtDataVisualization/Q3DScene>
#include <QtDataVisualization/Q3DCamera>
#include <QtDataVisualization/QScatter3DSeries>
#include <QtDataVisualization/Q3DTheme>
#include <QtCore/qmath.h>
#include <QtCore/QTextStream>
#include <QtCore/QDebug>
//-end- Qt_Official_sample:custominput

//-start- objfile-custominput
#include <QtDataVisualization/Q3DTheme>
#include <QtDataVisualization/QCustom3DItem>
#include <QtDataVisualization/QCustom3DLabel>
#include <QtGui/QImage>
//-start- objfile-custominput

using namespace QtDataVisualization;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_xRotation(0.0f)
    , m_yRotation(0.0f)
{
    ui->setupUi(this);
    this->setWindowTitle("3D obj-file . QtDataVisualization");
    func_GUIdefault(); //GUIフォーム初期化 rotation設定など

    //m_graph = new Q3DScatter();
    m_graph = new Q3DSurface();

    //-start- 旧scatter(散布図) 3Dグラフ基礎設定
    //m_graph->axisX()->setRange(-10, 10);
    //m_graph->axisY()->setRange(-10, 10);
    //m_graph->axisZ()->setRange(-10, 10);
    //QScatter3DSeries *series = new QScatter3DSeries;
    //series->setItemLabelFormat(QStringLiteral("@xLabel, @yLabel, @zLabel"));
    //series->setMesh(QAbstract3DSeries::MeshCube);
    //series->setItemSize(0.15f);
    //m_graph->addSeries(series); // m_graph->seriesList().at(0) の作成
    //-end- 旧scatter(散布図) 3Dグラフ基礎設定

    //-start- objfile-custominput
    m_graph->axisX()->setLabelFormat("%.1f N");
    m_graph->axisZ()->setLabelFormat("%.1f E");
    m_graph->axisX()->setRange(34.0f, 40.0f);
    m_graph->axisY()->setRange(0.0f, 200.0f);
    m_graph->axisZ()->setRange(18.0f, 24.0f);
    //
    m_graph->axisX()->setTitle(QStringLiteral("Latitude"));
    m_graph->axisY()->setTitle(QStringLiteral("Height"));
    m_graph->axisZ()->setTitle(QStringLiteral("Longitude"));
    //-end- objfile-custominput

    QWidget *container = QWidget::createWindowContainer(m_graph);

    if (!m_graph->hasContext()) {
        QMessageBox msgBox;
        msgBox.setText("Couldn't initialize the OpenGL context.");
        msgBox.exec();
        return;
    }

//    QScatter3DSeries *item = new QScatter3DSeries(); //vox表示の時、必須。
//    item->setMesh(QAbstract3DSeries::MeshUserDefined); //vox表示の時、必須。
//    addData(); //csvファイルからデータを読み込んで追加

    //设置到控件上　コントロールに設定→ Widgetに登録、画面で表示されるようになる
    QHBoxLayout *hLayout = new QHBoxLayout();
    hLayout->addWidget(container, 1);
    ui->widget->setLayout(hLayout);

    //-start- 視点移動ができるように設定 Qt-Official-sample bars
    changePresetCamera();
    // Set up property animations for zooming to the selected bar
    Q3DCamera *camera = m_graph->scene()->activeCamera();
    m_defaultAngleX = camera->xRotation();
    m_defaultAngleY = camera->yRotation();
    m_defaultZoom = camera->zoomLevel();
    m_defaultTarget = camera->target();

    m_animationCameraX.setTargetObject(camera);
    m_animationCameraY.setTargetObject(camera);
    m_animationCameraZoom.setTargetObject(camera);
    m_animationCameraTarget.setTargetObject(camera);

    m_animationCameraX.setPropertyName("xRotation");
    m_animationCameraY.setPropertyName("yRotation");
    m_animationCameraZoom.setPropertyName("zoomLevel");
    m_animationCameraTarget.setPropertyName("target");

    int duration = 1700;
    m_animationCameraX.setDuration(duration);
    m_animationCameraY.setDuration(duration);
    m_animationCameraZoom.setDuration(duration);
    m_animationCameraTarget.setDuration(duration);

    // The zoom always first zooms out above the graph and then zooms in
    qreal zoomOutFraction = 0.3;
    m_animationCameraX.setKeyValueAt(zoomOutFraction, QVariant::fromValue(0.0f));
    m_animationCameraY.setKeyValueAt(zoomOutFraction, QVariant::fromValue(90.0f));
    m_animationCameraZoom.setKeyValueAt(zoomOutFraction, QVariant::fromValue(50.0f));
    m_animationCameraTarget.setKeyValueAt(zoomOutFraction,
    QVariant::fromValue(QVector3D(0.0f, 0.0f, 0.0f)));
    //-end- 視点移動ができるように設定 Qt-Official-sample bars

    //-start- [comment-out]objfile-custominput 3D画面表示されたobjファイルアイテムクリック時にアニメーションする設定だが不要なので
    //connect(m_graph, &QAbstract3DGraph::selectedElementChanged,
    //        this, &MainWindow::handleElementSelected);
    //m_selectionAnimation = new QPropertyAnimation(this);
    //m_selectionAnimation->setPropertyName("scaling");
    //m_selectionAnimation->setDuration(500);
    //m_selectionAnimation->setLoopCount(-1);
    //-end- [comment-out]objfile-custominput 3D画面表示されたobjファイルアイテムクリック時にアニメーションする設定だが不要なので

}

MainWindow::~MainWindow()
{
    //qDebug() << "Widget::~Widget()";
    delete m_graph;
    delete ui;
}

//-start-  視点移動 Qt-official-sample bar より流用
void MainWindow::changePresetCamera()
{
    m_animationCameraX.stop();
    m_animationCameraY.stop();
    m_animationCameraZoom.stop();
    m_animationCameraTarget.stop();

    // Restore camera target in case animation has changed it
    m_graph->scene()->activeCamera()->setTarget(QVector3D(0.0f, 0.0f, 0.0f));

    //! [10]
    static int preset = Q3DCamera::CameraPresetFront;

    m_graph->scene()->activeCamera()->setCameraPreset((Q3DCamera::CameraPreset)preset);

    if (++preset > Q3DCamera::CameraPresetDirectlyBelow)
        preset = Q3DCamera::CameraPresetFrontLow;
    //! [10]
}

void MainWindow::on_rotationSliderX_valueChanged(int value)
{
    rotateX(value);
}

void MainWindow::on_rotationSliderY_valueChanged(int value)
{
    rotateY(value);
}

void MainWindow::rotateX(int rotation) //3D視点移動
{
    m_xRotation = rotation;
    m_graph->scene()->activeCamera()->setCameraPosition(m_xRotation, m_yRotation);
}

void MainWindow::rotateY(int rotation) //3D視点移動
{
    m_yRotation = rotation;
    m_graph->scene()->activeCamera()->setCameraPosition(m_xRotation, m_yRotation);
}

void MainWindow::func_GUIdefault() //GUIフォームの初期化
{
    //QSlider *rotationSliderX = new QSlider(Qt::Horizontal, widget);
    ui->rotationSliderX->setTickInterval(30);
    ui->rotationSliderX->setTickPosition(QSlider::TicksBelow);
    ui->rotationSliderX->setMinimum(-180);
    ui->rotationSliderX->setValue(0);
    ui->rotationSliderX->setMaximum(180);
    //QSlider *rotationSliderY = new QSlider(Qt::Horizontal, widget);
    ui->rotationSliderY->setTickInterval(15);
    ui->rotationSliderY->setTickPosition(QSlider::TicksAbove);
    ui->rotationSliderY->setMinimum(-90);
    ui->rotationSliderY->setValue(0);
    ui->rotationSliderY->setMaximum(90);
}
//-end-  視点移動 Qt-official-sample bar より流用


void MainWindow::func_onCheckBox1_3Ddraw_objfile(bool show) //objファイルを3D画面に描画する
{
    //流用元: Qt公式サンプルcustominputs　ToggleItemThreeより流用
    QVector3D positionThree = QVector3D(34.5f, 86.0f, 19.1f);
    QVector3D positionThreeLabel = QVector3D(34.5f, 116.0f, 19.1f);
    if (show) {
        QImage color = QImage(2, 2, QImage::Format_RGB32);
        color.fill(Qt::darkMagenta);
        QCustom3DItem *item = new QCustom3DItem();
        //item->setMeshFile(":/items/refinery.obj");
        item->setMeshFile("C:/kuroda/work/Qt/qt_my_datavisual_01/QtDataVisual_my_3Dobj/data/refinery.obj");
        item->setPosition(positionThree);
        item->setScaling(QVector3D(0.04f, 0.04f, 0.04f));
        item->setRotation(QQuaternion::fromAxisAndAngle(0.0f, 1.0f, 0.0f, 75.0f));
        item->setTextureImage(color);
        item->setShadowCasting(false);
        m_graph->addCustomItem(item);

        //QImage color = QImage(2, 2, QImage::Format_RGB32);
        //color.fill(Qt::Green);
        color = QImage(2, 2, QImage::Format_ARGB32); //半透明表示
        color.fill(QColor(0, 255, 0, 128)); //半透明表示
        QCustom3DItem *item2 = new QCustom3DItem();
        //item2->setMeshFile("C:/kuroda/work/Qt/qt_my_datavisual_01/QtDataVisual_my_customitems/sample_plane_02.obj");
        item2->setMeshFile("C:/kuroda/work/Qt/qt_my_datavisual_01/QtDataVisual_my_3Dobj/data/oilrig.obj");
        item2->setPosition(positionThree);
        item2->setScaling(QVector3D(0.05f, 0.05f, 0.05f));
        item2->setRotation(QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 0.0f, 0.0f)); //(x,y,z,角度degree) xyz中心として角度移動する
        item2->setTextureImage(color);
        item2->setShadowCasting(false);
        m_graph->addCustomItem(item2);

//        color = QImage(2, 2, QImage::Format_RGB32);
//        color.fill(Qt::blue);
//        QCustom3DItem *item3 = new QCustom3DItem();
//        item3->setMeshFile("C:/kuroda/work/Qt/qt_my_datavisual_01/QtDataVisual_my_customitems/sample_plane_bottom.obj");
//        item3->setPosition(positionThree);
//        item3->setScaling(QVector3D(0.05f, 0.05f, 0.05f));
//        item3->setRotation(QQuaternion::fromAxisAndAngle(0.0f, 0.0f, 0.0f, 0.0f)); //(x,y,z,角度degree) xyz中心として角度移動する
//        item3->setTextureImage(color);
//        item3->setShadowCasting(false);
//        m_graph->addCustomItem(item3);


        QCustom3DLabel *label = new QCustom3DLabel();
        label->setText("Refinery");
        label->setPosition(positionThreeLabel);
        label->setScaling(QVector3D(0.5f, 0.5f, 0.5f));
        m_graph->addCustomItem(label);
    } else {
        //resetSelection();
        m_graph->removeCustomItemAt(positionThree);
        m_graph->removeCustomItemAt(positionThreeLabel);
    }
}

//-start- 要? 不要? objfile-custominput
//void MainWindow::handleElementSelected(QAbstract3DGraph::ElementType type)
//{
//    resetSelection();
//    if (type == QAbstract3DGraph::ElementCustomItem) {
//        QCustom3DItem *item = m_graph->selectedCustomItem();
//        QString text;
//        if (qobject_cast<QCustom3DLabel *>(item) != 0) {
//            text.append("Custom label: ");
//        } else {
//            QStringList split = item->meshFile().split("/");
//            text.append(split.last());
//            text.append(": ");
//        }
//        int index = m_graph->selectedCustomItemIndex();
//        text.append(QString::number(index));
//        m_textField->setText(text);
//        m_previouslyAnimatedItem = item;
//        m_previousScaling = item->scaling();
//        m_selectionAnimation->setTargetObject(item);
//        m_selectionAnimation->setStartValue(item->scaling());
//        m_selectionAnimation->setEndValue(item->scaling() * 1.5f);
//        m_selectionAnimation->start();
//    } else if (type == QAbstract3DGraph::ElementSeries) {
//        QString text = "Surface (";
//        QSurface3DSeries *series = m_graph->selectedSeries();
//        if (series) {
//            QPoint point = series->selectedPoint();
//            QString posStr;
//            posStr.setNum(point.x());
//            text.append(posStr);
//            text.append(", ");
//            posStr.setNum(point.y());
//            text.append(posStr);
//        }
//        text.append(")");
//        m_textField->setText(text);
//    } else if (type > QAbstract3DGraph::ElementSeries
//               && type < QAbstract3DGraph::ElementCustomItem) {
//        int index = m_graph->selectedLabelIndex();
//        QString text;
//        if (type == QAbstract3DGraph::ElementAxisXLabel)
//            text.append("Axis X label: ");
//        else if (type == QAbstract3DGraph::ElementAxisYLabel)
//            text.append("Axis Y label: ");
//        else
//            text.append("Axis Z label: ");
//        text.append(QString::number(index));
//        m_textField->setText(text);
//    } else {
//        m_textField->setText("Nothing");
//    }
//}

//void MainWindow::resetSelection() //不要? 3D画面 描画リセット
//{
//    m_selectionAnimation->stop();
//    if (m_previouslyAnimatedItem)
//        m_previouslyAnimatedItem->setScaling(m_previousScaling);
//    m_previouslyAnimatedItem = 0;
//}

void MainWindow::on_checkBox1_stateChanged(int arg1)
{
    func_onCheckBox1_3Ddraw_objfile(true); //objファイル表示
}