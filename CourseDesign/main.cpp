#include <GLUT/GLUT.h>
#include <iostream>
#include <cmath>
#include <cstdio>
using namespace std;
#define LEFT_EDGE   1
#define RIGHT_EDGE  2
#define BOTTOM_EDGE 4
#define TOP_EDGE    8
#define DEGREES_TO_RADIANS 3.14159 / 180.0
// 旋转参数
static GLfloat spin = 0.0;
GLfloat x = 0.0;
GLfloat y = 0.0;
GLfloat size = 50.0;
GLfloat angle = 2.0;
GLsizei wh = 500, ww = 500;

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


/*画正方形*/
void square() {
    glBegin(GL_QUADS);
    glVertex2f(x+320, y+240);
    glVertex2f(-y+320, x+240);
    glVertex2f(-x+320, -y+240);
    glVertex2f(y+320, -x+240);
    glEnd();
}
/*旋转*/
void spinDisplay(void) {
    spin = spin + 2.0;
    if (spin > 360.0) spin = spin - 360.0;
    x = 125.0 * cos(DEGREES_TO_RADIANS * spin);
    y = 125.0 * sin(DEGREES_TO_RADIANS * spin);
    glutPostRedisplay();
}

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

/*直线以及画圆*/
void LineDDA(int x0, int y0, int x1, int y1)
{
   int dx, dy, epsl, k;
   float x, y, xIncre, yIncre;
   dx = x1 - x0;
   dy = y1 - y0;
   x = x0; y = y0;
   if (abs(dx) > abs(dy)) epsl = abs(dx);
   else epsl = abs(dy);
   xIncre = (float)dx / (float)epsl;
   yIncre = (float)dy / (float)epsl;
   glColor3f (1.0f, 1.0f, 0.0f);
   glPointSize(3);
   for(k = 0; k <= epsl; k++)
   {
       glBegin (GL_POINTS);
       glVertex2i (x, (int)(y+0.5));
       glEnd ();
       x += xIncre;
       y += yIncre;
   }
}

void Bresenham(int x0, int y0, int x1, int y1) {
   int dx, dy, d, UpIncre, DownIncre, x, y;
   int flag = 0;
   if (abs(y1 - y0) > abs(x1 - x0)) {
       swap(x0, y0);
       swap(x1, y1);
       flag = 1;
   }
   if (x0 > x1){
       swap(x1, x0);
       swap(y1, y0);
   }
   x = x0; y = y0;
   dx = x1 - x0; dy = y1 - y0;
   d = dx - 2 * dy;
   UpIncre = 2 * dx - 2 * dy;
   DownIncre = -2 * dy;
   glColor3f (0.0f, 1.0f, 0.0f);
   glPointSize(3);
   if (y0 < y1) {
       while (x <= x1) {
           glBegin(GL_POINTS);
           if (flag == 0) // 0 < k < 1
               glVertex2i(x, y);
           else
               glVertex2i(y, x);
           glEnd();
           x++;
           if (d < 0) {
               y++;
               d += UpIncre;
           } else {
               d += DownIncre;
           }
       }
   } else {
       d = -dx - 2 * dy;
       UpIncre = -2 * dx - 2 * dy;
       DownIncre = -2 * dy;
       while (x <= x1) {
           glBegin(GL_POINTS);
           if (flag == 0) // 0 < k < 1
               glVertex2i(x, y);
           else
               glVertex2i(y, x);
           glEnd();
           if (d < 0) {
               x++;
               d += DownIncre;
           } else {
               x++;
               y--;
               d += UpIncre;
           }
       }
   }
}

// circle
void eighth_circle(int x0, int y0, int r) {
   int x = 0;
   int y = r;
   int d = 1 - r;
   glColor3f(1.0f, 0.0f, 1.0f);
   glPointSize(3);
   while (x < y) {
       glBegin(GL_POINTS);
       glVertex2i(x + x0, y + y0);
       glVertex2i(y + x0, x + y0);
       glVertex2i(x + x0, -y + y0);
       glVertex2i(-y + x0, x + y0);
       glVertex2i(-x + x0, y + y0);
       glVertex2i(y + x0, -x + y0);
       glVertex2i(-x + x0, -y + y0);
       glVertex2i(-y + x0, -x + y0);
       glEnd();
       x++;
       if (d < 0) d += 2 * x + 3;
       else {
           d += 2*(x-y) + 5;
           y--;
       }
   }

}

/*多边形裁剪*/
//点的结构体，x和y为点的坐标
typedef struct
{
    float x, y;
}polygon2D;

//多边形的顶点坐标
static polygon2D points[15] = {};

//多边形的顶点数
static int Clip_N = 0;

//多边形裁剪窗口坐标
static polygon2D wMin, wMax;

//多边形裁剪窗口边界对应着0、1、2、3
typedef enum
{
    Left, Right, Bottom, Top
}Boundary;

//判断顶点在不在裁剪窗口中
int inside(polygon2D p, Boundary b, polygon2D wMin, polygon2D wMax)
{
    switch (b)
    {
        case Left:
            if (p.x < wMin.x) return (false);
            break;
        case Right:
            if (p.x > wMax.x) return (false);
            break;
        case Bottom:
            if (p.y < wMin.y) return (false);
            break;
        case Top:
            if (p.y > wMax.y) return (false);
            break;
    }
    return true;
}

//判断直线是否穿过了裁剪窗口
int cross(polygon2D p1, polygon2D p2, Boundary b, polygon2D wMin, polygon2D wMax)
{
    if (inside(p1, b, wMin, wMax) == inside(p2, b, wMin, wMax))
        return false;
    else
        return true;
}


//按左、右、下、上边界来确定裁剪后的顶点
polygon2D intersect(polygon2D p1, polygon2D p2, Boundary b, polygon2D wMin, polygon2D wMax)
{
    polygon2D temp;
    float m = 0;
    if (p1.x != p2.x) m = (p2.y - p1.y) / (p2.x - p1.x);
    switch (b) {
        case Left:
            temp.x = wMin.x;
            temp.y = p2.y + (wMin.x - p2.x) * m;
            break;
        case Right:
            temp.x = wMax.x;
            temp.y = p2.y + (wMax.x - p2.x) * m;
            break;
        case Bottom:
            temp.y = wMin.y;
            if (p1.x != p2.x)temp.x = p2.x + (wMin.y - p2.y) / m;
            else temp.x = p2.x;
            break;
        case Top:
            temp.y = wMax.y;
            if (p1.x != p2.x) temp.x = p2.x + (wMax.y - p2.y) / m;
            else temp.x = p2.x;
            break;
    }
    return temp;
}

//多边形裁剪，并更新裁剪后的顶点数组pOut
int edgeCliper(Boundary b, polygon2D wMin, polygon2D wMax, polygon2D* pIn, int cnt, polygon2D* pOut)
{
    polygon2D s;
    int i, Ocnt = 0;
    s = pIn[0];
    for (i = 1; i <= cnt; i++)
    {
        if (!inside(s, b, wMin, wMax) && inside(pIn[i], b, wMin, wMax))
        {
            pOut[Ocnt] = intersect(s, pIn[i], b, wMin, wMax);
            Ocnt++;
            pOut[Ocnt] = pIn[i];
            Ocnt++;
        }
        else if (inside(s, b, wMin, wMax) && inside(pIn[i], b, wMin, wMax))
        {
            pOut[Ocnt] = pIn[i];
            Ocnt++;
        }
        else if (inside(s, b, wMin, wMax) && (!inside(pIn[i], b, wMin, wMax)))
        {
            pOut[Ocnt] = intersect(s, pIn[i], b, wMin, wMax);
            Ocnt++;
        }
        s = pIn[i];
    }
    return (Ocnt);
}
// 多边形初始化函数
void  init_polygon(void)
{
    glClearColor(1.0, 1.0, 1.0, 0.0);
    wMin = { 200,200 };
    wMax = { 400,400 };
}

//多边形裁剪主函数
void ClipPolygon(void)
{
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    int i, cnt, Ocnt, b;
    cnt = Clip_N;
    polygon2D pOut[60], pIn[60];
    //初始化输入点、裁剪后的输出点数组
    for (i = 0; i < 60; i++)
    {
        pIn[i].x = 0.0;
        pIn[i].y = 0.0;
        pOut[i].x = 0.0;
        pOut[i].y = 0.0;
    }
    
    for (i = 0; i <= cnt; i++)
    {
        //为了绘制多边形时可以首尾相连，所以设置终点和起点坐标相同
        if (i == cnt)
        {
            pIn[i] = points[0];
        }
        else
        {
            pIn[i] = points[i];
        }
    }
    glLineWidth(2.0);
    glColor3f(0.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    glVertex2f(wMin.x, wMin.y);
    glVertex2f(wMax.x, wMin.y);
    glVertex2f(wMax.x, wMax.y);
    glVertex2f(wMin.x, wMax.y);
    glEnd();

    
    if (Clip_N > 2)
    {
        //绘制多边形的控制点
        glColor3f(0.0, 0.0, 1.0);
        glPointSize(6.0f); //画点的大小
        glBegin(GL_POINTS);
        for (i = 0; i < cnt; i++)
            glVertex2f(pIn[i].x, pIn[i].y);
        glEnd();
        //绘制多边形
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        for (i = 0; i < cnt; i++)
            glVertex2f(pIn[i].x, pIn[i].y);
        glEnd();
    }
    else if (Clip_N == 1)
    {
        //printf("%f %f\n", pIn[0].x, pIn[0].y);
        glColor3f(0.0, 0.0, 1.0);
        glPointSize(6.0f); //画点的大小
        glBegin(GL_POINTS);
        glVertex2f(pIn[0].x, pIn[0].y);
        glEnd();
    }
    else if (Clip_N == 2)
    {
        glColor3f(0.0, 0.0, 1.0);
        glPointSize(2.0f); //画点的大小
        glBegin(GL_POINTS);
        glVertex2f(pIn[0].x, pIn[0].y);
        glVertex2f(pIn[1].x, pIn[1].y);
        glEnd();
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_STRIP);
        glVertex2f(pIn[0].x, pIn[0].y);
        glVertex2f(pIn[1].x, pIn[1].y);
        glEnd();
    }
    //按左、右、下、上边界来逐边裁剪
    //前面的结果是后面的输出
    for (b = 0; b < 4; b++)
    {
        Ocnt = edgeCliper(Boundary(b), wMin, wMax, pIn, cnt, pOut);
        for (i = 0; i < Ocnt; i++)
            pIn[i] = pOut[i];
        pIn[Ocnt] = pOut[0]; // 首尾相连
        cnt = Ocnt;
    }
    // 画出裁剪后的直线
    glColor3f(1.0, 0.0, 0.0);
    glBegin(GL_LINE_LOOP);
    for (i = 0; i < cnt; i++)
        glVertex2f(pOut[i].x, pOut[i].y);
    glEnd();
    glFlush();
}


void Reshape(int w, int h)
{
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, (GLsizei)w, (GLsizei)h, 0.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        ww = w;
        wh = h;
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
        switch (key)
            {
                case'c':
                    // 清空多边形
                    Clip_N = 0; //多边形的个数
                    for (int i = 0; i < 15; i++)
                    {
                        points[i].x = 0;
                        points[i].y = 0;
                    }
                    glutPostRedisplay();
                    break;
                case'd':case'D':
                    printf("请输入新的裁剪窗口的左下和右上角坐标:例如200 200 400 400\n");
                    scanf("%f %f %f %f", &wMin.x, &wMin.y, &wMax.x, &wMax.y);
                    glutPostRedisplay();
                    break;
                default:
                    break;
                
            }
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
        if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
                return;
        if (Clip_N < 15)
        {
            //获得鼠标点击的坐标
            points[Clip_N].x = x;
            points[Clip_N].y = y;
            //重绘
            glutPostRedisplay();
            Clip_N++;//多边形顶点数加1
        }
    } else if (flag == 6) {
        if (button == GLUT_RIGHT_BUTTON && state == GLUT_DOWN) exit(0);
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN)
            glutIdleFunc(spinDisplay);
        if (button == GLUT_MIDDLE_BUTTON && state == GLUT_DOWN) glutIdleFunc(NULL);
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
        glLineWidth(2.0);
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(0.0, 0.0, 0.0);
        glBegin(GL_LINE_LOOP);
        glVertex2f(rect.xmin, rect.ymin);
        glVertex2f(rect.xmin, rect.ymax);
        glVertex2f(rect.xmax, rect.ymax);
        glVertex2f(rect.xmax, rect.ymin);
        glEnd();
        if (bDrawLine)
            LineGL(x2, y2, x3, y3);
        glFlush();
        flag = 1;
    } else if (imode == 2) {
        /*绘制直线*/
        glClear(GL_COLOR_BUFFER_BIT);
           glPointSize(30);

           LineDDA(200, 400, 400, 0); // k < -1

           LineDDA(0, 200, 400, 000); // 0 > k > -1

           LineDDA(0, 200, 400, 400); // 0 < k < 1

           LineDDA(200, 0, 400, 400); // k > 1
        
           Bresenham(0, 0, 200, 400);
           Bresenham(0, 0, 400, 200);
           Bresenham(0, 400, 200, 0);
           Bresenham(0, 400, 400, 200);
           eighth_circle(200, 200, 200);
           glFlush();
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
        ClipPolygon();
        flag = 5;
    } else if (imode == 6) {
        /*动态绘制矩形*/
        glClear(GL_COLOR_BUFFER_BIT);
        glColor3f(0.0, 0.0, 0.0);
        square();
        glutSwapBuffers();
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
    glutAddMenuEntry("直线以及八点画圆", 2);
//    int nGLutCircleMenu = glutCreateMenu(ProcessMenu);
//    glutAddMenuEntry("动态绘圆", 3);
//    int nGLutTCMenu = glutCreateMenu(ProcessMenu);
//    glutAddMenuEntry("动态绘椭圆", 4);
    int nGLutMCMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态多边形裁剪", 5);
    int nGLutrectMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("二维正方形变换", 6);
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
    glutAddSubMenu("直线以及八点画圆", nGLutBesierMenu);
//    glutAddSubMenu("动态绘圆", nGLutCircleMenu);
//    glutAddSubMenu("动态绘椭圆", nGLutTCMenu);
    glutAddSubMenu("动态多边形裁剪", nGLutMCMenu);
    glutAddSubMenu("二维正方形变换", nGLutrectMenu);
    glutAddSubMenu("二维变换绘制", nGLutchangeMenu);
    glutAddSubMenu("二次均匀B样条曲线绘制", nGLutBT2Menu);
    glutAddSubMenu("三次均匀B样条曲线绘制", nGLutBT3Menu);
    glutAddSubMenu("动态三角形绘制", nGLuttraiMenu);
    glutAttachMenu(GLUT_RIGHT_BUTTON);
    glMatrixMode(GL_PROJECTION);
    
    init_line();
    init_Bspline();
    init_polygon();
    glutDisplayFunc(Display);
    glutKeyboardFunc(keyboard);
    glutMouseFunc(mouse);
    glutMotionFunc(motion);
    glutReshapeFunc(Reshape);
    glutIdleFunc(spinDisplay);
//    glutSpecialFunc(SpecialKeys);
    glutMainLoop();
    return 0;
}
