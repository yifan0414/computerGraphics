#include <GLUT/GLUT.h>
#include <iostream>
#include <cstdio>
using namespace std;
#define LEFT_EDGE   1
#define RIGHT_EDGE  2
#define BOTTOM_EDGE 4
#define TOP_EDGE    8


int flag = 0;
int imode; // 菜单所选的功能
/*直线裁剪算法所用数据结构*/
struct MyRect
{
    float xmin, xmax, ymin, ymax;
};

MyRect rect;
int x0, y0, x1, y1;
int px0, py0, px1, py1;
bool bDrawLine = true;
int width = 640, height = 480;
/*直线裁剪算法所用数据结构*/

void LineGL(int x0, int  y0, int x1, int y1)
{
    glBegin(GL_LINES);
    glColor3f(1.0f, 0.0f, 0.0f);   glVertex2f(x0, y0);
    glColor3f(1.0f, 0.0f, 0.0f);   glVertex2f(x1, y1);
    glEnd();
}

//求p点的编码
int CompCode(int x, int y, MyRect rect)
{
    int code = 0x00;
    if (y > rect.ymax)
        code = code | 8;// 1000
    if (y < rect.ymin)
        code = code | 4;// 0100
    if (x > rect.xmax)
        code = code | 2;
    if (x < rect.xmin)
        code = code | 1;
    return code;
}

bool cohenSutherlandLineClip(MyRect  rect, int &x0, int & y0, int &x1, int &y1)
{
    bool accept, done;
    float x = 0, y = 0;
    accept = false;
    done = false;

    int code0, code1, codeout;
    code0 = CompCode(x0, y0, rect);
    code1 = CompCode(x1, y1, rect);
    do {
        if (!(code0 | code1))
        {
            accept = true;
            done = true;
        }
        else if (code0 & code1)
            done = true;
        else
        {
            if (code0 != 0)
                codeout = code0;
            else
                codeout = code1;

            if (codeout&LEFT_EDGE) {
                y = y0 + (y1 - y0)*(rect.xmin - x0) / (x1 - x0);
                x = (float)rect.xmin;
            }
            else if (codeout&RIGHT_EDGE) {
                y = y0 + (y1 - y0)*(rect.xmax - x0) / (x1 - x0);
                x = (float)rect.xmax;
            }
            else if (codeout&BOTTOM_EDGE) {
                x = x0 + (x1 - x0)*(rect.ymin - y0) / (y1 - y0);
                y = (float)rect.ymin;
            }
            else if (codeout&TOP_EDGE) {
                x = x0 + (x1 - x0)*(rect.ymax - y0) / (y1 - y0);
                y = (float)rect.ymax;
            }

            if (codeout == code0)
            {
                x0 = x; y0 = y;
                code0 = CompCode(x0, y0, rect);
            }
            else
            {
                x1 = x; y1 = y;
                code1 = CompCode(x1, y1, rect);
            }
        }
    } while (!done);
    return accept;
}

// liang-barsky
bool LBLineClipTest(GLfloat p, GLfloat q, GLfloat &umax, GLfloat &umin) {
    GLfloat r = 0.0;
    if (p < 0.0) {
        r = q / p;
        if (r > umin) return false;
        else if (r > umax) umax = r;
    } else if (p > 0.0) {
        r = q / p;
        if (r < umax) return false;
        if (r < umin) umin = r;
    } else if (q < 0.0) return false;
    return true;
}

bool LBLineClip(MyRect rect, int &x0, int &y0, int &x1, int &y1) {
    GLfloat umax, umin, deltax, deltay;
    deltax = x1 - x0; deltay = y1 - y0; umax = 0.0; umin = 1;
    if (LBLineClipTest(-deltax, x0 - rect.xmin, umax, umin)) {
        if (LBLineClipTest(deltax, rect.xmax - x0, umax, umin)) {
            if (LBLineClipTest(-deltay, y0 - rect.ymin, umax, umin)) {
                if (LBLineClipTest(deltay, rect.ymax - y0, umax, umin)) {
                    int temp1 = x0, temp2 = y0; // 这是一个bug，因为 x0 和 x1 的值会改变
                    x0 = int(x0 + umax * deltax + 0.5);
                    y0 = int(y0 + umax * deltay + 0.5);
                    x1 = int(temp1 + umin * deltax + 0.5);
                    y1 = int(temp2 + umin * deltay + 0.5);
                    return true;
                }
            }
        }
    }
    return false;
}


void myDisplay()
{
    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(1.0f, 1.0f, 0.0f);
    glRectf(rect.xmin, rect.ymin, rect.xmax, rect.ymax);

    if (bDrawLine)
        LineGL(x0, y0, x1, y1);

    glFlush();
}

void init_line()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
    glShadeModel(GL_FLAT);

    rect.xmin = 100; //窗口的大小
    rect.xmax = 300;
    rect.ymin = 100;
    rect.ymax = 300;
    x0 = 300; y0 = 50; x1 = 50; y1 = 450;
    px0 = x0; py0 = y0; px1 = x1; py1 = y1;

    glClear(GL_COLOR_BUFFER_BIT);
    glColor3f(0.0f, 1.0f, 0.0f);
    glRectf(rect.xmin, rect.ymin, rect.xmax, rect.ymax);

}

void restore() {
    x0 = px0; x1 = px1; y0 = py0; y1 = py1;
}

void Reshape(int w, int h)
{
    glViewport(0, 0, (GLsizei)w, (GLsizei)h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)w, 0.0, (GLdouble)h);
}

void keyboard(unsigned char key, int x, int y)
{
    if (flag == 1) {
        switch (key)
        {
            case 'c':
                bDrawLine = cohenSutherlandLineClip(rect, x0, y0, x1, y1);
//                bDrawLine = LBLineClip(rect, x0, y0, x1, y1);
                cout << x0 << " " << y0 << " " << x1 << " " << y1 << endl;
                glutPostRedisplay();
                break;
            case 'a':
                printf("请输入裁剪矩形的坐标：\n");
                scanf("%f%f%f%f", &rect.xmin, &rect.ymin, &rect.xmax, &rect.ymax);
                glutPostRedisplay();
                break;
            case 'r':
                restore();
                glutPostRedisplay();
                break;
            default:
                break;
        }
    }
}

void mouse(int button, int state, int x, int y)
{
    if (flag == 1) {
        if (button == GLUT_LEFT_BUTTON) {
                if (state == GLUT_DOWN)
                {
                    if (bDrawLine)
                    {
                        x0 = x;
                        y0 = height - y;
                        bDrawLine = false;
                        glutPostRedisplay();
                    }
                    else
                    {
                        x1 = x;
                        y1 = height - y;
                        bDrawLine = true;
                        glutPostRedisplay();
                    }
                }
                px0 = x0; px1 = x1; py0 = y0; py1 = y1; // 存储之前的直线坐标，用于恢复直线
        }
    }
}

void Display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    //绕x轴旋转
    //绕y轴旋转
    //glShadeModel(GL_SMOOTH);
    /*指定要绘制的图元*/
    if (imode == 1) {
        /*动态直线裁剪*/
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(1.0f, 1.0f, 0.0f);
        glRectf(rect.xmin, rect.ymin, rect.xmax, rect.ymax);
        if (bDrawLine)
            LineGL(x0, y0, x1, y1);
        glFlush();
            
        flag = 1;
    } else if (imode == 2) {
        /*绘制贝塞尔曲线*/

        flag = 2;
        //Initial();
    } else if (imode == 2) {
        /*动态绘圆*/
        flag = 3;
    } else if (imode == 2) {
        /*动态绘椭圆*/


        flag = 4;
    } else if (imode == 2) {
        /*动态多边形裁剪*/

        flag = 5;
    } else if (imode == 3) {
        /*动态绘制矩形*/
        
        flag = 6;
    } else if (imode == 5) {
        /*二维变换绘制*/
        
        flag = 7;
    } else if (imode == 6) {
        /*二次均匀B样条绘制*/
    
        flag = 8;
    } else if (imode == 7) {
        /*二次均匀B样条绘制*/
        flag = 9;
    } else if (imode == 9) {
        /*动态三角形绘制*/

        flag = 10;
    }
    glFlush();
}

void ProcessMenu(int value) {
    //选择绘制模式
    imode = value;
    glutPostRedisplay();
}



int main(int argc, char *argv[])
{
    glutInit(&argc, argv);
    glutInitDisplayMode(GLUT_RGB | GLUT_SINGLE);
    glutInitWindowPosition(100, 100);
    glutInitWindowSize(width, height);
    glutCreateWindow("计科182苏一凡");
    
    int nGLutLine_Clip_Menu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("直线裁剪", 1);
    int nGLutBesierMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("贝塞尔曲线绘制", 2);
    int nGLutCircleMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态绘圆", 3);
    int nGLutTCMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态绘椭圆", 4);
    int nGLutMCMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态多边形裁剪", 5);
    int nGLutrectMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态绘制矩形", 6);
    int nGLutchangeMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("二维变换绘制", 7);
    int nGLutBT2Menu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("二次均匀B样条曲线绘制", 8);
    int nGLutBT3Menu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("三次均匀B样条曲线绘制", 9);
    int nGLuttraiMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态三角形绘制", 10);


    /*创建主菜单*/
    int nMainMenu = glutCreateMenu(ProcessMenu);
    glutAddSubMenu("直线裁剪", nGLutLine_Clip_Menu);
    glutAddSubMenu("贝塞尔曲线绘制", nGLutBesierMenu);
    glutAddSubMenu("动态绘圆", nGLutCircleMenu);
    glutAddSubMenu("动态绘椭圆", nGLutTCMenu);
    glutAddSubMenu("动态多边形裁剪", nGLutMCMenu);
    glutAddSubMenu("动态绘制矩形", nGLutrectMenu);
    glutAddSubMenu("二维变换绘制", nGLutchangeMenu);
    glutAddSubMenu("二次均匀B样条曲线绘制", nGLutBT2Menu);
    glutAddSubMenu("三次均匀B样条曲线绘制", nGLutBT3Menu);
    glutAddSubMenu("动态三角形绘制", nGLuttraiMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glMatrixMode(GL_PROJECTION);
    
    init_line();
    glutDisplayFunc(Display);
    glutReshapeFunc(Reshape);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
//    glutSpecialFunc(SpecialKeys);
    glutMainLoop();
    return 0;
}




