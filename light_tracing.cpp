#include <stdio.h>
#include <string>
#include <string.h>
#include <cstring>
#include <iostream>
#include <cmath>
#include <vector>
#include <windows.h>
#include <string.h>
#include <conio.h>
#include <stdlib.h>
using namespace std;

namespace Rain_Kotsuzui
{
    const double pi = acos(-1);
    const double eps = 0.01;
    long ground_light = 1;
    // Todo: 类名大驼峰,实例名小驼峰
    struct vec
    {
        double eps_ = 0.01;
        double x, y, z;
        vec(double a = 0, double b = 0, double c = 0)
        {
            x = a, y = b, z = c;
        };
        double Length()
        {
            return sqrt(x * x + y * y + z * z);
        }
        vec Unit() const
        {
            double l = sqrt(x * x + y * y + z * z);
            return vec(x, y, z) / l;
        }

        double operator*(const vec& A) const
        {
            return x * A.x + y * A.y + z * A.z;
        }
        vec operator*(const double k) const
        {
            vec res;
            res.x = this->x * k;
            res.y = this->y * k;
            res.z = this->z * k;
            return res;
        }
        vec operator/(const double k) const
        {
            vec res;
            res.x = this->x / k;
            res.y = this->y / k;
            res.z = this->z / k;
            return res;
        }
        vec operator+(const vec& A) const
        {
            vec res;
            res.x = this->x + A.x;
            res.y = this->y + A.y;
            res.z = this->z + A.z;
            return res;
        }
        vec operator-(const vec& A) const
        {
            vec res;
            res.x = this->x - A.x;
            res.y = this->y - A.y;
            res.z = this->z - A.z;
            return res;
        }

        static vec crs(const vec &A, const vec &B)
        {
            vec res;
            res.x = A.y * B.z - A.z * B.y;
            res.y = -A.x * B.z + A.z * B.x;
            res.z = A.x * B.y - A.y * B.x;
            return res;
        }
    };
    struct Camera
    {
        double ang = pi / 3; // 半视角
        double f = 10;       // 焦距
        double m = f * tan(ang);

        double sight = 200; // 视距
        double step = 0.1;
        double ang_step = pi / 45;

        vec pos = vec(0, 0, 0);

        double theta = 0, phi = pi / 2;
        // phi=[0,pi],theta=[0,2pi]

        vec direct = vec(1, 0, 0);
        vec e_x;
        vec e_y;

        void set()
        {
            if (theta >= 2 * pi)
                theta -= 2 * pi;
            if (theta < 0)
                theta += 2 * pi;
            m = f * tan(ang);
            ang_step = (pi / 45) * (ang / (pi / 3));
            direct = vec(cos(theta) * sin(phi), sin(theta) * sin(phi), cos(phi));
            e_x = vec(direct.y, -direct.x, 0);
            e_y = vec(-direct.x * direct.z, -direct.y * direct.z, 1 - direct.z * direct.z);
            e_x = e_x.Unit();
            e_y = e_y.Unit();
        }
    };
    struct Screen
    {
        char pix[210][210] = {};
        const char color[9] = { '@', '%', '#', '*', '+', '=', '-', '.', ' ' };
        //                      100,  90,  80,  70,  50,  30,  10,  5,   0
        void print(Camera cam)
        {
            HANDLE hOutput;
            COORD coord = { 0, 0 };
            hOutput = GetStdHandle(STD_OUTPUT_HANDLE);
            CONSOLE_CURSOR_INFO cci;
            GetConsoleCursorInfo(hOutput, &cci);
            cci.bVisible = false;
            SetConsoleCursorInfo(hOutput, &cci);
            string A = "\n";
            for (int j = 70; j >= 10; j--)
            {
                for (int i = 10; i <= 95; i++)
                {
                    // 理论上这里也可以多线程替换，但实际上性能瓶颈不在这里
                    A += pix[i][j];
                    A += ' ';
                }
                A += '\n';
            }
            SetConsoleCursorPosition(hOutput, coord);
            printf("\n%s", A.c_str());
            printf("now pos:%.2f %.2f %.2f ang:%.2f\n", cam.pos.x, cam.pos.y, cam.pos.z, cam.ang * 180 / pi);
        }

        void Color(double light, int i, int j)
        {
            if (light >= 330)
                pix[i][j] = color[0];
            else if (light >= 280)
                pix[i][j] = color[1];
            else if (light >= 230)
                pix[i][j] = color[2];
            else if (light >= 180)
                pix[i][j] = color[3];
            else if (light >= 130)
                pix[i][j] = color[4];
            else if (light >= 70)
                pix[i][j] = color[5];
            else if (light >= 30)
                pix[i][j] = color[6];
            else if (light >= 10)
                pix[i][j] = color[7];
            else
                pix[i][j] = color[8];
            return;
        }
    };

    struct Ball
    {
        Ball() {};
        Ball(vec pos, double r, double light)
            : pos(pos), r(r), light(light) {};
        vec pos = vec(0, 0, 0);
        double r = 0;
        double light = 100;
    };
    // ax^2+bx+c=0
    inline double delta(double a, double b, double c) { return b * b - 4 * a * c; }

    // Todo: i and j is not good name. Use wide_pixel height_pixel instead.
    void GetPixel(const Camera& cam, Screen& S, const std::vector<Ball> ball, int ball_num, int i, int j)
    {
        vec n = cam.direct * cam.f + cam.e_x * (i * 0.01 * cam.m) + cam.e_y * (j * 0.01 * cam.m);
        vec p = cam.pos;
        n = n.Unit();
        //
        double light = 0;
        int now_k = -1;
        double tot_lamda = 0;
        //
    again_:
        double lamda = -1;
        // ball
        for (int k = 0; k < ball_num; k++)
        {
            vec o = ball[k].pos;
            double r = ball[k].r;
            double del = delta(1, (n * (p - o)) * 2, (p - o) * (p - o) - r * r);
            if (k == now_k || del < 0)
                continue;
            if ((n * (o - p) - sqrt(del) / 2 > 0) && (lamda < 0 || lamda > n * (o - p) - sqrt(del) / 2))
            {
                now_k = k, lamda = n * (o - p) - sqrt(del) / 2;
            }
        }
        // ground
        if (n.z < 0)
        {
            double t = abs(p.z / n.z);
            vec o = p + n * t;
            if (lamda < 0 || lamda > t)
            {
                tot_lamda += t;
                if ((((int)o.x / 1) % 2) ^ (((int)o.y / 1) % 2))
                    light += ground_light / cbrt(tot_lamda * tot_lamda);
                S.Color(light, i + 60, j + 50);
                return;
            }
        }

        // color
        if (lamda < 0 || tot_lamda > cam.sight)
            S.Color(light, i + 60, j + 50);
        else
        {
            tot_lamda += lamda;
            light += ball[now_k].light / (sqrt(tot_lamda));
            double temp = (p + (n * lamda) - ball[now_k].pos) * (p - ball[now_k].pos) / (ball[now_k].r * ball[now_k].r);
            vec tem = n;
            n = (p + (n * lamda) - ball[now_k].pos) * (2 * temp - 1) - (p - ball[now_k].pos);
            n = n.Unit();
            p = p + tem * lamda;
            // First todo: remove goto(goto will cause lots of effert.)
            goto again_;
        }
        return;
    }

    void GetPicture(const Camera &camera, Screen& screen, const std::vector<Ball> &ball, int ball_num)
    {
        //[i,j]
        // 优化瓶颈，可以用多线程或GPU解决
        for (int i = -50; i <= 50; i++)
            for (int j = -50; j <= 50; j++)
                GetPixel(camera, screen, ball, ball_num, i, j);
    }

    void Move(Camera& camera)
    {
        char key;
        camera.set();
        if (_kbhit())
        {
            fflush(stdin);
            key = _getch();
            vec direct;
            switch (key)
            {
            case 'w':
                direct = camera.direct;
                direct.z = 0;
                direct = direct.Unit();
                camera.pos = camera.pos + direct * camera.step;
                break;
            case 's':
                direct = camera.direct;
                direct.z = 0;
                direct = direct.Unit();
                camera.pos = camera.pos - direct * camera.step;
                break;
            case 'a':
                direct = camera.e_x;
                direct.z = 0;
                direct = direct.Unit();
                camera.pos = camera.pos - direct * camera.step;
                break;
            case 'd':
                direct = camera.e_x;
                direct.z = 0;
                direct = direct.Unit();
                camera.pos = camera.pos + direct * camera.step;
                break;
            case 'q':
                camera.pos = camera.pos + vec(0, 0, 1) * camera.step;
                break;
            case 'e':
                camera.pos = camera.pos - vec(0, 0, 1) * camera.step;
                break;
                // up
            case 72:
                if (camera.phi > camera.ang_step)
                    camera.phi -= camera.ang_step;
                break;
                // down
            case 80:
                if (camera.phi <= pi - camera.ang_step)
                    camera.phi += camera.ang_step;
                break;
                // right
            case 77:
                camera.theta -= camera.ang_step;
                break;
                // left
            case 75:
                camera.theta += camera.ang_step;
                break;
                // 视角
            case ',':
                if (camera.ang < pi / 2 - eps)
                    camera.ang += camera.ang_step * 0.4;
                break;
            case '.':
                if (camera.ang > 0)
                    camera.ang -= camera.ang_step * 0.4;
                break;
            default:
                break;
            }
        }
    }

    void BallMove(std::vector<Ball> balls, double time)
    {
        balls[1].pos = vec(8 * cos(time * 0.1), 8 * sin(time * 0.1), 0.5);
        balls[1].light = 300 + 100 * sin(time * 2);
        balls[2].pos = vec(8 * cos(time * 1), 7 * sin(time * 2), 3);
        balls[6].pos = vec(8, 7, 3);
        balls[6].light = 400 + 200 * sin(time * 2);
        balls[4].pos = vec(8 * cos(time * 2), 7 * sin(time * 2), 10);
        balls[5].pos = vec(100 * cos(time * 0.01), 70 * sin(time * 0.05), 50 + 50 * cos(time * 0.05) * sin(time * 0.05));
        balls[0].pos = vec(0, -8 + 0.1 * cos(time * pi), 4 + 3 * sin(time * 0.1));
    }

    void main()
    {
        Screen S;
        Camera cam;
        cam.pos = vec(0, 0, 1);

        std::vector<Ball> balls = {
            // pos, radius, light
            Ball(vec(0, -8, 4), 5, 300),
            Ball(vec(8, 0, 0), 1, 2000),
            Ball(vec(8, 0, 0), 1.5, 300),
            Ball(vec(0, 2, 4), 5, 100),
            Ball(vec(0, 0, 10), 2, 1000),
            Ball(vec(-100, 0, 50), 50, 100),
            Ball(vec(8, 0, 0), 1.5, 300)
        };
        const int ball_num = static_cast<int>(balls.size());
        double time = 0; // Todo: it is not a good name, and will error if run days.
        while (1)
        {
            Move(cam);
            BallMove(balls, time);
            GetPicture(cam, S, balls, ball_num);
            S.print(cam);
            // cout << "\033c";
            time += 0.005;
            ground_light = 1000 + static_cast<long>(500 * sin(time * 2));
            // Todo: 主线程（渲染线程）整体1秒，而不是主操作完后等一秒。这样渲染效果更佳
            Sleep(1);
        }
        return;
    }
}
int main()
{
    Rain_Kotsuzui::main();
    return 0;
}
