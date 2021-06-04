#include <GLUT/GLUT.h>
#include <iostream>
#include <cmath>
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
int x2, y2, x3, y3;
int px2, py2, px3, py3;
bool bDrawLine = true;
int width = 640, height = 480;
/*直线裁剪算法所用数据结构*/


/*B样条曲线所用数据结构*/
#define NUM_POINTS 4
#define NUM_SEGMENTS NUM_POINTS - 2 // 阶数

int cnt = NUM_POINTS; // 这里表示目前有多少个点

struct Point2
{
    double x;
    double y;

    Point2() { ; }
    Point2(int px, int py) { x = px; y = py; }
    void SetPoint2(int px, int py) { x = px; y = py; }
};

/*全局变量*/
Point2 vec[100];
bool mouseLeftDown = false;//记录鼠标左右键事件触发
/*B样条曲线所用数据结构*/


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


/*绘制B样条曲线*/
void Bspline(int n) {
    float f1, f2, f3;
    float deltaT = 1.0 / n;
    float T;

    glBegin(GL_LINE_STRIP);
    // NUM_SEGMENTS = 点数 - 2（二阶）
    for (int num = 0; num < cnt - 2; num++) {
        for (int i = 0; i <= n; i++) {
            T = i * deltaT;
            f1 = (1-T) *(1-T) / 2;
            f2 = (T + 1)*(1 - T) / 2 + (T) * (2 - T) / 2;
            f3 = T * T / 2;

            glVertex2f(f1*vec[num].x + f2*vec[num+1].x + f3*vec[num+2].x,
                       f1*vec[num].y + f2*vec[num+1].y + f3*vec[num+2].y);
        }
    }
    glEnd();
}

double distance(int x1, int y1, int x2, int y2) {
    return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}

void init_line()
{
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glShadeModel(GL_FLAT);

    rect.xmin = 100; //窗口的大小
    rect.xmax = 300;
    rect.ymin = 100;
    rect.ymax = 300;
    x2 = 300; y2 = 50; x3 = 50; y3 = 450;
    px2 = x2; py2 = y2; px3 = x3; py3 = y3;
}

void init_Bspline() {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glShadeModel(GL_FLAT);

    vec[0].SetPoint2(100, 300);
    vec[1].SetPoint2(200, 100);
    vec[2].SetPoint2(400, 100);
    vec[3].SetPoint2(500, 300);
}




void Reshape(int w, int h)
{
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, (GLsizei)w, (GLsizei)h, 0.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
}

void motion(int x, int y) {
    if (flag == 8) { // Bspline
        if (mouseLeftDown) {
            int min_point = 0;
            for (int i = 0; i < cnt; i++) {
                if (distance(vec[min_point].x, vec[min_point].y, x, y) > distance(vec[i].x, vec[i].y, x, y)) min_point = i;
            }
            vec[min_point].SetPoint2(x, y);
        }
    }
    glutPostRedisplay();
}


void keyboard(unsigned char key, int x, int y)
{
    if (flag == 1) {
        switch (key)
        {
            case 'c':
                bDrawLine = cohenSutherlandLineClip(rect, x2, y2, x3, y3);
//                bDrawLine = LBLineClip(rect, x0, y0, x1, y1);
                cout << x2 << " " << y2 << " " << x3 << " " << y3 << endl;
                glutPostRedisplay();
                break;
            case 'a':
                printf("请输入裁剪矩形的坐标：\n");
                scanf("%f%f%f%f", &rect.xmin, &rect.ymin, &rect.xmax, &rect.ymax);
                glutPostRedisplay();
                break;
            case 'r':
                x2 = px2; x3 = px3; y2 = py2; y3 = py3;
                glutPostRedisplay();
                break;
            default:
                break;
        }
    } else if (flag == 2) {
        
    } else if (flag == 3) {
        
    } else if (flag == 4) {
        
    } else if (flag == 5) {
        
    } else if (flag == 6) {
        
    } else if (flag == 7) {
        
    } else if (flag == 8) {
        switch (key)
        {
            case 'c':
                vec[cnt].x = x;
                vec[cnt].y = y;
                cnt++;
                glutPostRedisplay();
                break;
            case 'r':
                cnt--; // 懒惰删除
                glutPostRedisplay();
                break;
            default:
                break;
        }
    } else if (flag == 9) {
        
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
                        x2 = x;
                        y2 = y;
                        bDrawLine = false;
                        glutPostRedisplay();
                    }
                    else
                    {
                        x3 = x;
                        y3 = y;
                        bDrawLine = true;
                        glutPostRedisplay();
                    }
                }
                px2 = x2; px3 = x3; py2 = y2; py3 = y3; // 存储之前的直线坐标，用于恢复直线
        }
    } else if (flag == 2) {
        
    } else if (flag == 3) {
        
    } else if (flag == 4) {
        
    } else if (flag == 5) {
        
    } else if (flag == 6) {
        
    } else if (flag == 7) {
        
    } else if (flag == 8) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            mouseLeftDown = true;
        }

        if (button == GLUT_LEFT_BUTTON && state == GLUT_UP) {
            mouseLeftDown = false;
        }
    } else if (flag == 9) {
        
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
        glColor3f(0.5f, 0.5f, 1.0f);
        glRectf(rect.xmin, rect.ymin, rect.xmax, rect.ymax);
        if (bDrawLine)
            LineGL(x2, y2, x3, y3);
        glFlush();
        flag = 1;
    } else if (imode == 2) {
        /*绘制贝塞尔曲线*/

        flag = 2;
        //Initial();
    } else if (imode == 3) {
        /*动态绘圆*/
        flag = 3;
    } else if (imode == 4) {
        /*动态绘椭圆*/


        flag = 4;
    } else if (imode == 5) {
        /*动态多边形裁剪*/

        flag = 5;
    } else if (imode == 6) {
        /*动态绘制矩形*/
        
        flag = 6;
    } else if (imode == 7) {
        /*二维变换绘制*/
        
        flag = 7;
    } else if (imode == 8) {
        /*二次均匀B样条绘制*/
        glClear(GL_COLOR_BUFFER_BIT);

        glLineWidth(1.5f);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINE_STRIP);
        for (int i = 0;i < cnt; i++) {
            glVertex2f(vec[i].x, vec[i].y);
        }
        glEnd();

        glPointSize(10.0f);
        glColor3f(0.0, 0.0, 1.0);
        glBegin(GL_POINTS);
        for (int i = 0; i < cnt; i++) {
            glVertex2f(vec[i].x, vec[i].y);
        }
        glEnd();
        Bspline(20);
        glFlush();
        glutSwapBuffers();
        flag = 8;
    } else if (imode == 9) {
        /*二次均匀B样条绘制*/
        flag = 9;
    } else if (imode == 10) {
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
    init_Bspline();
    glutDisplayFunc(Display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(Reshape);
//    glutSpecialFunc(SpecialKeys);
    glutMainLoop();
    return 0;
}




