#include "Newpson/Viewer/GLWidget/glwidget.h"
#include <QtWidgets/QOpenGLWidget>

#include <QWidget>
#include <QSurfaceFormat>
#include <QVector>
#include <QVector3D>
#include <QMouseEvent>
#include <QGuiApplication>

#include "Newpson/Obj/Parser/parser.h"
#include "Newpson/Mesh/mesh.h"

namespace Newpson::Viewer {

GLWidget::GLWidget(QWidget *parent):
    QOpenGLWidget(parent)
{
    QSurfaceFormat fmt;
    fmt.setDepthBufferSize(24);
    fmt.setSamples(4);
    fmt.setVersion(2, 0);
    fmt.setProfile(QSurfaceFormat::CoreProfile);
    setFormat(fmt);
}

void GLWidget::initializeGL()
{
    initializeOpenGLFunctions();
    glClearColor(0.0, 0.0, 0.0, 1.0);

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Vertex,
        "#version 150\n"
        " \n"
        "in vec3 position; \n"
        "in vec3 normal; \n"
        "out vec3 frag_normal; \n"
        "out vec3 frag_camera; \n"
        "uniform mat4 view; \n"
        "uniform mat4 projection; \n"
        "uniform vec3 camera; \n"
        " \n"
        "void main() \n"
        "{ \n"
        "    frag_camera = camera; \n"
        "    frag_normal = normal; \n"
        "    gl_Position = projection * view * vec4(position, 1.0); \n"
        "} \n")) {
        qDebug() << "Vertex shader compilation error:" << m_program.log();
        return;
    }

    if (!m_program.addShaderFromSourceCode(QOpenGLShader::Fragment,
        "#version 150 \n"
        "\n"
        "const vec3 sun = vec3(-1.0, -1.0, -1.0); \n"
        "in highp vec3 frag_camera; \n"
        "in highp vec3 frag_normal; \n"
        " \n"
        "void main() \n"
        "{ \n"
        // "    vec3 reflected = reflect(sun, frag_normal); \n"
        // "    float intensity = max(0.1, -dot(reflected, frag_camera)); \n"
        // "    float intensity = clamp(1.0 - pow(dot(-sun, frag_normal), 3.0), 0.1, 1.0);"
        "    float intensity = clamp(dot(-normalize(frag_camera), normalize(frag_normal)), 0.1, 1.0);"
        "    gl_FragColor = vec4(intensity, intensity, intensity, 1.0); \n"
        "} \n")) {
        qDebug() << "Fragment shader compilation error:" << m_program.log();
        return;
    }

    m_program.bindAttributeLocation("position", 0);
    m_program.bindAttributeLocation("normal", 1);

    if (!m_program.link()) {
        qDebug() << "Program linkage error:" << m_program.log();
        return;
    }

    if (!m_program.bind())  {
        qDebug() << "Program binding error:" << m_program.log();
        return;
    }

    m_projectionLoc = m_program.uniformLocation("projection");
    m_viewLoc = m_program.uniformLocation("view");
    m_dirLoc = m_program.uniformLocation("camera");
    m_vbo.create();
    m_vbo.bind();

    const float *data = m_rawData.data();

    m_vbo.allocate(data, m_rawData.length() * sizeof(float));
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(0 * sizeof(GLfloat)));
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), reinterpret_cast<void *>(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    m_vbo.release();

    m_projection.setToIdentity();
    m_program.setUniformValue(m_viewLoc, m_camera.view());
    m_program.setUniformValue(m_dirLoc, m_camera.direction());

    m_program.release();
}

void GLWidget::paintGL()
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);

    m_program.bind();
    /*
        for (int i = 0; i < renderableTargets; ++i)
        {
            glDrawArrays(GL_TRIANGLES, targets.starts[i], targets.lengths[i]);
        }
    */
    m_program.setUniformValue(m_viewLoc, m_camera.view());
    m_program.setUniformValue(m_dirLoc, m_camera.direction());
    glDrawArrays(GL_TRIANGLES, 0, m_rawData.length() / 6);
    m_program.release();
}

void GLWidget::resizeGL(int width, int height)
{
    m_projection.setToIdentity();
    m_projection.perspective(45.0f, (float) width / height, 0.01f, 1000.0f);
    m_program.bind();
    m_program.setUniformValue(m_projectionLoc, m_projection);
}

void GLWidget::mousePressEvent(QMouseEvent *event)
{
    m_mousePositionLast = event->pos();
    if (event->button() == Qt::MouseButton::LeftButton) {
        if (QGuiApplication::keyboardModifiers() == Qt::ShiftModifier) {
            m_movementType = MOVEMENT_SLIDE;
            return;
        }
        m_movementType = MOVEMENT_ROTATION;
        return;
    }
    m_movementType = MOVEMENT_NONE;
}

void GLWidget::mouseMoveEvent(QMouseEvent *event)
{
    const QPointF mousePosition = event->pos();
    const QPointF mouseMotion = mousePosition - m_mousePositionLast;

    switch (m_movementType) {
    case MOVEMENT_ROTATION:
        m_camera.rotate(mouseMotion);
        update();
        break;
    case MOVEMENT_SLIDE:
        m_camera.slide(mouseMotion);
        update();
        break;
    case MOVEMENT_NONE:
        break;
    default:
        break;
    }

    m_mousePositionLast = mousePosition;
}

void GLWidget::wheelEvent(QWheelEvent *event)
{
    m_camera.zoom(event->angleDelta().y());
    update();
}

}
