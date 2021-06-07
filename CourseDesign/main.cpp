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
GLfloat xp = 320, yp = 240;

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
    glVertex2f(x + xp, y + yp);
    glVertex2f(-y + xp, x + yp);
    glVertex2f(-x + xp, -y + yp);
    glVertex2f(y + xp, -x + yp);
    glEnd();
}

//绘制一个菱形
void drawDiamond(void)
{
    glBegin (GL_POLYGON);
    glVertex2f (0,0);        //左下点 (200,200)
    glVertex2f (20,-40);        //右下点 (40,280)
    glVertex2f (0,-80);        //右上点 (200,360)
    glVertex2f (-20,-40);       //左上点 (160,280)
    glEnd ();
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
int cnt2 = 5;
struct Point2
{
    double x;
    double y;

    Point2() { ; }
    Point2(int px, int py) { x = px; y = py; }
    void SetPoint2(int px, int py) { x = px; y = py; }
};

/*全局变量*/
Point2 vec[100], vec2[100];
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
            //从最低位开始枚举（左右下上），找到第一个1，求出交点
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
    } else if (q < 0.0) return false; //证明此时的 p = 0，直线为垂直线
    return true;
}

bool LBLineClip(MyRect rect, int &x0, int &y0, int &x1, int &y1) {
    GLfloat umax, umin, deltax, deltay;
    deltax = x1 - x0; deltay = y1 - y0; umax = 0.0; umin = 1;
    if (LBLineClipTest(-deltax, x0 - rect.xmin, umax, umin)) {
        if (LBLineClipTest(deltax, rect.xmax - x0, umax, umin)) {
            if (LBLineClipTest(-deltay, y0 - rect.ymin, umax, umin)) {
                if (LBLineClipTest(deltay, rect.ymax - y0, umax, umin)) {
                    //上面四次判断即保证了直线穿过了裁剪窗口，也求出了umax和umin
                    int temp1 = x0, temp2 = y0; // 这里需要用同步更新
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
void init_Bspline2() {
    glClearColor(1.0, 1.0, 1.0, 0.0);
    glShadeModel(GL_FLAT);
    vec2[0].SetPoint2(100, 400);
    vec2[1].SetPoint2(200, 200);
    vec2[2].SetPoint2(300, 200);
    vec2[3].SetPoint2(400, 400);
    vec2[4].SetPoint2(500, 400);
    //vec[5].SetPoint2(600, 300);
}

// 开放均匀B样条曲线
void Bspline2(int n) {
    float f1, f2, f3;
    float deltaT = 1.0 / n;
    float T;

    glBegin(GL_LINE_STRIP);
    // glBegin(GL_POINTS);
    // 根据基函数的重叠位置，分别带入控制点坐标进行计算
    int num = 0;
    for (int i = 0; i <= n; i++) {
        T = i * deltaT;
        f1 = pow(1 - T, 2);
        f2 = T / 2 * (4 - 3 * T);
        f3 = pow(T, 2) / 2;

        glVertex2f(f1 * vec2[num].x + f2 * vec2[num + 1].x + f3 * vec2[num + 2].x,
                   f1 * vec2[num].y + f2 * vec2[num + 1].y + f3 * vec2[num + 2].y);
    }
    num = 1;
    for (int i = 0; i <= n; i++) {
        T = i * deltaT + 1;
        f1 = pow(2 - T, 2) / 2;
        f2 = T / 2 * (2 - T) + (T - 1) * (3 - T) / 2;
        f3 = pow(T - 1, 2) / 2;

        glVertex2f(f1 * vec2[num].x + f2 * vec2[num + 1].x + f3 * vec2[num + 2].x,
                   f1 * vec2[num].y + f2 * vec2[num + 1].y + f3 * vec2[num + 2].y);
    }
    num = 2;
    for (int i = 0; i <= n; i++) {
        T = i * deltaT + 2;
        f1 = pow(3 - T, 2) / 2;
        f2 = (3 * T - 5) * (3 - T) / 2;
        f3 = pow(T - 2, 2);

        glVertex2f(f1 * vec2[num].x + f2 * vec2[num + 1].x + f3 * vec2[num + 2].x,
                   f1 * vec2[num].y + f2 * vec2[num + 1].y + f3 * vec2[num + 2].y);
    }
    glEnd();
}

/*直线生成算法DDA*/
void DDA(int x0, int y0, int x1, int y1)
{
    int dx, dy, epsl, k;
    float x, y, xIncre, yIncre;
    dx = x1 - x0;
    dy = y1 - y0;
    x = x0; y = y0;
    // 这里是关键的一步，通过判断斜率来确定沿着x轴或者y轴作为步长方向
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
// 直线生成算法
void Bresenham(int x0, int y0, int x1, int y1) {
    int dx, dy, d, UpIncre, DownIncre, x, y;
    int flag = 0;
    // 如果斜率大于 1，就交换x和y的值，并做标记
    if (abs(y1 - y0) > abs(x1 - x0)) {
        swap(x0, y0);
        swap(x1, y1);
        flag = 1;
    }
    // 使得x0 < x1，确定增大方向
    if (x0 > x1){
        swap(x1, x0);
        swap(y1, y0);
    }
    x = x0; y = y0;
    dx = x1 - x0; dy = y1 - y0;
    glColor3f (0.0f, 1.0f, 0.0f);
    glPointSize(3);
    // 斜率>0
    if (y0 < y1) {
        d = dx - 2 * dy;
        UpIncre = 2 * dx - 2 * dy;
        DownIncre = -2 * dy;
        while (x <= x1) {
            glBegin(GL_POINTS);
            if (flag == 0) // 0 < k < 1
               glVertex2i(x, y);
            else           // k > 1
               glVertex2i(y, x);
            glEnd();
            x++;           // x方向+1
            if (d < 0) {
               y++;        // 如果d<0，意味着更靠近上方
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
            else           // k > 1
               glVertex2i(y, x);
            glEnd();
            x++;
            if (d < 0) {    // 如果d<0，意味着中点在直线（斜率为负数）下方，不增加
               d += DownIncre;
            } else {
               y--;         // 如果d>0，意味着中点在直线（斜率为负数）上方，减少1
               d += UpIncre;
            }
        }
    }
}

// 圆生成算法
void eighth_circle(int x0, int y0, int r) {
   int x = 0;
   int y = r;
   int d = 1 - r;
//   glColor3f(1.0f, 0.0f, 1.0f);
//   glPointSize(3);
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
        // 如果当前点在裁剪窗口外，下一个点在裁剪窗口内
        if (!inside(s, b, wMin, wMax) && inside(pIn[i], b, wMin, wMax))
        {
            pOut[Ocnt] = intersect(s, pIn[i], b, wMin, wMax); // 将交点加入输出表
            Ocnt++;
            pOut[Ocnt] = pIn[i];    // 将下一个顶点加入输出表
            Ocnt++;
        }
        // 如果当前点和下一个点都在裁剪窗口内
        else if (inside(s, b, wMin, wMax) && inside(pIn[i], b, wMin, wMax))
        {
            pOut[Ocnt] = pIn[i];
            Ocnt++;
        }
        //如果当前点在裁剪窗口内，下一个点不在裁剪窗口内
        else if (inside(s, b, wMin, wMax) && (!inside(pIn[i], b, wMin, wMax)))
        {
            pOut[Ocnt] = intersect(s, pIn[i], b, wMin, wMax); //将交点加入输出表
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

//二维几何变换
void twoDimensial(void)
{
    
//    xp = 320, yp = 240;
    glLoadIdentity();
    glPushMatrix(); // 保存变换矩阵
    glColor3f(1.0, 0.0, 0.0);
    glTranslatef(320, 80, 0);
    drawDiamond();

    glRotatef(120.0, 0.0, 0.0, 1.0);
    glColor3f(0.0, 1.0, 0.0);  // 绿色
    drawDiamond();

    glRotatef(120.0, 0.0, 0.0, 1.0);
    glColor3f(0.0, 0.0, 1.0);
    drawDiamond();
    glPopMatrix(); // 恢复变换矩阵
    float temp = 255.0;
    glPointSize(5);
    glColor3f(0,107/temp,176/temp);
    eighth_circle(100+xp-320, 220+yp-240, 50);
    glColor3f(29/temp, 24/temp, 21/temp);
    eighth_circle(180+xp-320, 220+yp-240, 50);
    glColor3f(220/temp, 47/temp, 31/temp);
    eighth_circle(260+xp-320, 220+yp-240, 50);
    glColor3f(239/temp, 169/temp, 13/temp);
    eighth_circle(140+xp-320, 270+yp-240, 50);
    glColor3f(5/temp, 147/temp, 65/temp);
    eighth_circle(220+xp-320, 270+yp-240, 50);
    glFlush();
}

const GLfloat PI = 3.1415926f;  //定义圆周率
//位置以及五角星一个外顶点坐标
void DrawStar(GLfloat px, GLfloat py, GLfloat vx, GLfloat vy, int flag)
{
    glBegin(GL_TRIANGLE_FAN);  //绘制一系列三角形
    GLfloat vtx[12], vty[12];
    // 1个中心点，10个顶点，顶点的第一个和最后一个相同，其中奇数下标是外顶点，偶数下标是内顶点
    vtx[0] = px;  //已知的中心点
    vty[0] = py;
    vtx[1] = vx;  //已知的第一个外顶点
    vty[1] = vy;
    //中心点到外顶点的长度
    GLfloat length1 = sqrt((px - vx) * (px - vx) + (py - vy) * (py - vy));
    //计算剩下的所有顶点
    GLfloat length2 = length1 * sin(0.1 * PI) / sin(126.0 / 180 * PI);
    double init = atan((vty[1] - vty[0]) /
                       (vtx[1] - vtx[0]));  //顶点与中心点连线与x轴的角度
    if (flag) init = init - PI;
    for (int i = 2; i < 12; i++) {
        init = init - 0.2 * PI;
        if (i % 2 == 0) {  //内顶点
            vtx[i] = length2 * cos(init) + vtx[0];
            vty[i] = length2 * sin(init) + vty[0];
        } else {  //外顶点
            vtx[i] = length1 * cos(init) + vtx[0];
            vty[i] = length1 * sin(init) + vty[0];
        }
    }
    for (int i = 0; i < 12; i++)  //设置顶点
        glVertex3f(vtx[i], vty[i], 0.5);
    glEnd();
}

// 贝塞尔曲线
//x,y 方向旋转参数
static int N = -1;   //贝赛尔曲线的幂次
static GLfloat Bfunc[15] = { 0.0 };    //Bernstein多项式的值的数组
GLfloat point[15][2] = { 0.0 };     //存储控制点的坐标
void Bezier()
{
    int i, j, t;
    GLfloat u;
    //使用的绘制点坐标
    GLfloat DPoint1[2];
    GLfloat DPoint2[2];
    //先将第一个控制点赋给第二个点，为后面的循环做准备
    for (i = 0; i < 2; i++)
        DPoint2[i] = point[0][i];
    glClear(GL_COLOR_BUFFER_BIT);
    //设置控制点的颜色
    glColor3f(1.0f, 0.0f, 0.0f);
    //设置控制点的大小
    glPointSize(5);
    //绘制控制点
    glBegin(GL_POINTS);
    for (i = 0; i <= N; i++)
    {
        glVertex2fv(point[i]);
    }
    glEnd();

    //设置连接控制点线的颜色
    glColor3f(0.0f, 0.0f, 0.0f);
    //设置连线的宽度
    glLineWidth(3);
    //绘制连线
    glBegin(GL_LINE_STRIP);
    for (i = 0; i <= N; i++)
        glVertex2fv(point[i]);
    glEnd();

    //设置Bezier曲线的颜色
    glColor3f(1.0f, 0.0f, 0.0f);
    //设置线宽
    glLineWidth(2);
    for (i = 0; i <= 1000; i++)
    {
        //获得u值
        u = i / 1000.0;
        //初始化Bfunc数组
        for (t = 0; t <= N; t++)
            Bfunc[t] = 1.0;
        //第一个点的坐标等于第二个点的坐标，方便下面的绘制
        DPoint1[0] = DPoint2[0];
        DPoint1[1] = DPoint2[1];
        //将第二个坐标的x，y设置为
        DPoint2[0] = 0.0;
        DPoint2[1] = 0.0;
        //循环、递推计算Bezier基函数的值
        for (j = 0; j <= N; j++)
        {
            if (j == 0)
            {
                //V0处的Bezier基函数
                Bfunc[j] = 1;
                for (t = N; t > j; t--)
                    Bfunc[j] = Bfunc[j] * (1 - u);
            }
            else
            {
                if (i != 1000)
                    Bfunc[j] = (1.0 * (N - j + 1) / j) * (u / (1 - u)) * Bfunc[j - 1];
                else
                {
                    //Bfunc[N]处的Bezier基函数
                    if (j == N)
                        for (t = 0; t < N; t++)
                            Bfunc[j] = Bfunc[j] * u;
                    else
                        Bfunc[j] = 0.0;
                }
            }
            //获得第二个点的坐标值
            DPoint2[0] = DPoint2[0] + Bfunc[j] * point[j][0];
            DPoint2[1] = DPoint2[1] + Bfunc[j] * point[j][1];
        }
        //连接两点
        if (N >= 1)
        {
            //printf("连接两点");
            glBegin(GL_LINES);
            glVertex2fv(DPoint1);
            glVertex2fv(DPoint2);
            glEnd();
        }
    }
    //glFlush();

}


void Reshape(int w, int h)
{
    
    if (flag == 3) {
        if (h == 0) h = 1;
        glViewport(0, 0, w, h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        int dis = w < h ? w : h;
        glViewport(0, 0, dis, dis);
        glOrtho(-1.5, 1.5, -1.5, 1.5, -1.5, 1.5);
        if (w <= h)
            glOrtho(-1.0, 1.0, 1.0, 1.0 * h / w, 1.0, -1.0);
        else
            glOrtho(-1.0, 1.0 * w / h, 1.0, 1.0, 1.0, -1.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
    } else {
        glViewport(0, 0, (GLsizei)w, (GLsizei)h);
        glMatrixMode(GL_PROJECTION);
        glLoadIdentity();
        gluOrtho2D(0.0, (GLsizei)w, (GLsizei)h, 0.0);
        glMatrixMode(GL_MODELVIEW);
        glLoadIdentity();
        ww = w;
        wh = h;
    }
}

void motion(int x, int y) {
    if (flag == 8 || flag == 9) { // Bspline
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
//                bDrawLine = cohenSutherlandLineClip(rect, x2, y2, x3, y3);
                bDrawLine = LBLineClip(rect, x2, y2, x3, y3);
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
        switch (key)
        {
            //退出运行系统
            case'q':case'Q':
                exit(0);
                break;
                //重画曲线
            case'c':case'C':
                N = -1;
                glutPostRedisplay();
                break;
            case 'r':
                N--;
                glutPostRedisplay();
                break;
            //刷新
            case'e':case'E':
                glutPostRedisplay();
                break;
            default:
                break;
        }
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
        
        //printf("贝塞尔鼠标点击事件", flag);
        //如果不是点击鼠标左键的状态，则不获得坐标值
        if (button != GLUT_LEFT_BUTTON || state != GLUT_DOWN)
            return;
        if (N < 100)
        {
            N++;
            //获得鼠标点击的坐标
        /*    point[N][0] = (2.0*x) / (float)(W - 1) - 1.0;
            point[N][1] = (2.0*(H - y)) / (float)(H)-1.0;*/
            point[N][0] = x;
            point[N][1] = y;
            //重绘
            //printf("进来了");
            glutPostRedisplay();
            //printf("贝塞尔鼠标点击事件结束");
        }
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
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            if (cnt2 < 5) {
                vec2[cnt2].x = x;
                vec2[cnt2].y = y;
                cnt++;
            } else {
                cnt2 = 0;
            }
        }
    }
}

void Display() {
    glClear(GL_COLOR_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    Reshape(640.0, 480.0);
    if (imode == 1) {
        /*动态直线裁剪*/
        glutSetWindowTitle("直线裁剪");
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
        glutSetWindowTitle("直线生成算法");
        glClear(GL_COLOR_BUFFER_BIT);
        glPointSize(30);
        DDA(200, 400, 400, 0); // k < -1
        DDA(0, 200, 400, 000); // 0 > k > -1
        DDA(0, 200, 400, 400); // 0 < k < 1
        DDA(200, 0, 400, 400); // k > 1
    
        Bresenham(0, 0, 200, 400);
        Bresenham(0, 0, 400, 200);
        Bresenham(0, 400, 200, 0);
        Bresenham(0, 400, 400, 200);
        glColor3f(1, 0, 1);
        eighth_circle(200, 200, 200);
        glFlush();
        flag = 2;
    } else if (imode == 3) {
        glutSetWindowTitle("五星红旗");
        Reshape(800, 600);
        glClear(GL_COLOR_BUFFER_BIT);  //完成清除窗口的任务
        //绘制红旗
        glColor3f(1, 0, 0);  //确定绘制物体时使用的颜色:红色
        glBegin(GL_QUADS);
        glVertex3f(-0.75, 0.5, 0.5);  //位于z=0.5平面的矩形 0.5是相对值
        glVertex3f(0.75, 0.5, 0.5);
        glVertex3f(0.75, -0.5, 0.5);
        glVertex3f(-0.75, -0.5, 0.5);
        glEnd();
        //绘制星星
        glColor3f(1.0, 1.0, 0.0);  //设置颜色为黄色
        GLfloat px[5] = {-1.5 / 3, -0.75 / 3, -0.75 / 5, -0.75 / 5, -0.75 / 3};
        GLfloat py[5] = {0.25, 0.4, 0.3, 0.15, 0.05};
        GLfloat vx[5] = {-1.5 / 3};
        GLfloat vy[5] = {0.4};  //五星的中心点和其中指定顶点
        //计算其余四星的顶点
        for (int i = 1; i < 5; i++) {
            vx[i] = px[i] - 0.05 * cos(atan((py[0] - py[i]) / (px[0] - px[i])));
            vy[i] = py[i] - 0.05 * sin(atan((py[0] - py[i]) / (px[0] - px[i])));
        }
        //绘制
        DrawStar(px[0], py[0], vx[0], vy[0], 0);
        DrawStar(px[1], py[1], vx[1], vy[1], 1);
        DrawStar(px[2], py[2], vx[2], vy[2], 1);
        DrawStar(px[3], py[3], vx[3], vy[3], 1);
        DrawStar(px[4], py[4], vx[4], vy[4], 1);
        glutSwapBuffers();  //交换缓冲区
        flag = 3;
    } else if (imode == 4) {
        glutSetWindowTitle("贝塞尔曲线");
//        puts("helloimode");
        Bezier();
        flag = 4;
    } else if (imode == 5) {
        /*动态多边形裁剪*/
        glutSetWindowTitle("动态多边形裁剪");
        ClipPolygon();
        flag = 5;
    } else if (imode == 6) {
        /*动态绘制矩形*/
        glClear(GL_COLOR_BUFFER_BIT);
        glutSetWindowTitle("正方形旋转");
        glColor3f(0.0, 0.0, 0.0);
        square();
        glutSwapBuffers();
        flag = 6;
    } else if (imode == 7) {
        gluOrtho2D(0.0, (GLsizei)1000, (GLsizei)600, 0.0);
        glutPostRedisplay();
        /*二维变换绘制*/
        glutSetWindowTitle("奥运五环");
        twoDimensial();
        flag = 7;
    } else if (imode == 8) {
        /*二次均匀B样条绘制*/
        glutSetWindowTitle("二次均匀B样条绘制");
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
        init_Bspline2();
        glClear(GL_COLOR_BUFFER_BIT);
        glLineWidth(1.5f);
        glColor3f(1.0, 0.0, 0.0);
        glBegin(GL_LINE_STRIP);
        for (int i = 0;i < cnt2; i++) {
            glVertex2f(vec2[i].x, vec2[i].y);
        }
        glEnd();

        glPointSize(10.0f);
        glColor3f(0.0, 0.0, 1.0);
        glBegin(GL_POINTS);
        for (int i = 0; i < cnt2; i++) {
            glVertex2f(vec2[i].x, vec2[i].y);
        }
        glEnd();
        Bspline2(20);
        glFlush();
        glutSwapBuffers();
        flag = 9;
    }
    glFlush();
}



void ProcessMenu(int value) {
    //选择绘制模式
    imode = value;
    glutPostRedisplay();
}

void SpecialKeys(int key, int x, int y) {

        //绕x轴旋转的角度变化
        if (key == GLUT_KEY_UP) yp -= 4.0f;
        if (key == GLUT_KEY_DOWN) yp += 4.0f;

        //绕y轴旋转的角度的变化
        if (key == GLUT_KEY_LEFT) xp -= 4.0f;
        if (key == GLUT_KEY_RIGHT) xp += 4.0f;
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
    int nGlutLineCircle = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("直线以及八点画圆", 2);
    int nGLutStar = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("五星红旗", 3);
    int nGLutBesier = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("贝塞尔曲线", 4);
    int nGLutMCMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("动态多边形裁剪", 5);
    int nGLutrectMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("二维正方形变换", 6);
    int nGLutchangeMenu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("二维变换绘制", 7);
    int nGLutBT2Menu = glutCreateMenu(ProcessMenu);
    glutAddMenuEntry("二次均匀B样条曲线绘制", 8);
//    int nGLutBT3Menu = glutCreateMenu(ProcessMenu);
//    glutAddMenuEntry("二次开放均匀B样条曲线绘制", 9);


    /*创建主菜单*/
    glutCreateMenu(ProcessMenu);
    glutAddSubMenu("直线裁剪", nGLutLine_Clip_Menu);
    glutAddSubMenu("直线以及八点画圆", nGlutLineCircle);
    glutAddSubMenu("五星红旗", nGLutStar);
    glutAddSubMenu("贝塞尔曲线", nGLutBesier);
    glutAddSubMenu("动态多边形裁剪", nGLutMCMenu);
    glutAddSubMenu("二维正方形变换", nGLutrectMenu);
    glutAddSubMenu("二维变换绘制", nGLutchangeMenu);
    glutAddSubMenu("二次均匀B样条曲线绘制", nGLutBT2Menu);
//    glutAddSubMenu("二次开放均匀B样条曲线绘制", nGLutBT3Menu);
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
    glutSpecialFunc(SpecialKeys);
    glutMainLoop();
    return 0;
}
