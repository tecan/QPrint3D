#include "oglwidget.h"
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QMouseEvent>

OGLWidget::OGLWidget(QWidget* pParent)
    : QOpenGLWidget(pParent)
    , clearColor(Qt::black)
    , xRot(0)
    , yRot(0)
    , zRot(0)
    , program(NULL){
    memset(textures, 0, sizeof(textures));
}

OGLWidget::~OGLWidget(){
    makeCurrent();
    vbo.destroy();
    for(int i=0; i<6; i++){
        delete textures[i];
    }
    delete program;
    doneCurrent();
}

QSize OGLWidget::minimumSizeHint() const {
    return QSize(50, 50);
}

QSize OGLWidget::sizeHint() const {
    return QSize(200, 200);
}

void OGLWidget::rotateBy(int xAngle, int yAngle, int zAngle){
    xRot += xAngle;
    yRot += yAngle;
    zRot += zAngle;
    update();
}

void OGLWidget::setClearColor(const QColor &color){
    clearColor = color;
    update();
}

void OGLWidget::initializeGL(){
    initializeOpenGLFunctions();
    makeObject();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

#define PROGRAM_VERTEX_ATTRIBUTE 0
#define PROGRAM_TEXCOORD_ATTRIBUTE 1
    QOpenGLShader* vshader = QOpenGLShader(QOpenGLShader::Vertex, this);
    const char *vsrc =
        "attribute highp vec4 vertex;\n"
        "attribute mediump vec4 texCoord;\n"
        "varying mediump vec4 texc;\n"
        "uniform mediump mat4 matrix;\n"
        "void main(void)\n"
        "{\n"
        "    gl_Position = matrix * vertex;\n"
        "    texc = texCoord;\n"
        "}\n";
    vshader->compileSourceCode(vsrc);

    QOpenGLShader *fshader = new QOpenGLShader(QOpenGLShader::Fragment, this);
    const char *fsrc =
        "uniform sampler2D texture;\n"
        "varying mediump vec4 texc;\n"
        "void main(void)\n"
        "{\n"
        "    gl_FragColor = texture2D(texture, texc.st);\n"
        "}\n";
    fshader->compileSourceCode(fsrc);

    program = new QOpenGLShaderProgram();
    program->addShader(vshader);
    program->addShader(fshader);
    program->bindAttributeLocation("vertex", PROGRAM_VERTEX_ATTRIBUTE);
    program->bindAttributeLocation("texCoord", PROGRAM_TEXCOORD_ATTRIBUTE);
    program->link();

    program->bind();
    program->setUniformValue("texture", 0);
}

void OGLWidget::paintGL(){
    glClearColor(clearColor.redF(), clearColor.greenF(), clearColor.blueF());
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    QMatrix4x4 m;
    m.ortho(-0.5f, 0.5f, 0.5f, -0.5f, 4.0f, 15.0f);
    m.translate(0.0f, 0.0f, -10.0f);
    m.rotate(xRot/16.0f, 1.0f, 0.0, 0.0f);
    m.rotate(yRot/16.0f, 0.0f, 1.0f, 0.0f);
    m.rotate(zRot/16.0f, 0.0f, 0.0f, 1.0f);

    program->setUniformValue("matrix", m);
    program->enableAttributeArray(PROGRAM_VERTEX_ATTRIBUTE);
    program->enableAttributeArray(PROGRAM_TEXCOORD_ATTRIBUTE);
    program->setAttributeBuffer(PROGRAM_VERTEX_ATTRIBUTE, GL_FLOAT, 0, 3, 5 * sizeof(GLfloat));
    program->setAttributeBuffer(PROGRAM_TEXCOORD_ATTRIBUTE, GL_FLOAT, 3 * sizeof(GLfloat), 2, 5 * sizeof(GLfloat));

    for(int i=0; i < 6; i++){
        textures[i]->bind();
        glDrawArrays(GL_TRIANGLE_FAN, i * 4, 4);
    }
}

void OGLWidget::resizeGL(int width, int height){
    int side = qMin(width, height);
    glViewport((width - side)/2, (height - side)/2, side, side);
}

void OGLWidget::mousePressEvent(QMouseEvent *event){
    lastPos = event->pos();
}

void OGLWidget::mouseMoveEvent(QMouseEvent *event){
    int dx = event->x() - lastPos.x();
    int dy = event->y() - lastPos.y();

    if(event->buttons() & Qt::LeftButton){
        rotateBy(8*dy, 8*dx, 0);
    }else if(event->buttons() & Qt::RightButton){
        rotateBy(8*dy, 0, 8*dx);
    }
    lastPos = event->pos();
}

void OGLWidget::mouseReleaseEvent(QMouseEvent *event){
    emit clicked();
}

void OGLWidget::makeObject(){
    static const int coords[6][4][3] = {
        { { +1, -1, -1 }, { -1, -1, -1 }, { -1, +1, -1 }, { +1, +1, -1 } },
        { { +1, +1, -1 }, { -1, +1, -1 }, { -1, +1, +1 }, { +1, +1, +1 } },
        { { +1, -1, +1 }, { +1, -1, -1 }, { +1, +1, -1 }, { +1, +1, +1 } },
        { { -1, -1, -1 }, { -1, -1, +1 }, { -1, +1, +1 }, { -1, +1, -1 } },
        { { +1, -1, +1 }, { -1, -1, +1 }, { -1, -1, -1 }, { +1, -1, -1 } },
        { { -1, -1, +1 }, { +1, -1, +1 }, { +1, +1, +1 }, { -1, +1, +1 } }
    };
    for(int i=0; i<6; i++){
        textures[i] = new QOpenGLTexture(QImage(QString("c:/images/slide%1.png").arg(i+1)).mirrored());
    }
    QVector<GLfloat> vertData;
    for(int i=0; i<6; i++){
        for(int j=0; j<6; j++){
            vertData.append(0.2 * coords[i][j][0]);
            vertData.append(0.2 * coords[i][j][1]);
            vertData.append(0.2 * coords[i][j][2]);

            //texture coordinate
            vertData.append(j == 0 || j == 3);
            vertData.append(j == 0 || j == 1);
        }
    }
    vbo.create();
    vbo.allocate(vertData.constData(), vertData.count() * sizeof(GLfloat));
}
