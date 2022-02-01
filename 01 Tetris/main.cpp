#include <SFML/Graphics.hpp>
#include <time.h>
using namespace sf;

const int M = 20;
const int N = 10;

const int tileSize = 18;

int field[M][N] = {0};

struct Point
{ int x, y; } a[4], b[4];

int figures[7][4] =
{
  1,3,5,7, // I
  2,4,5,7, // Z
  3,5,4,6, // S
  3,5,4,7, // T
  2,3,5,7, // L
  3,5,7,6, // J
  2,3,4,5, // O
};

bool check()
{
  for (int i=0;i<4;i++) {
    if (a[i].x<0 || a[i].x>=N || a[i].y>=M) return 0;
    else if (field[a[i].y][a[i].x]) return 0;
  }
  return 1;
}

void printA() {
  for (int i=0;i<4;i++) {
    printf("a.x:%i | a.y:%i\n", a[i].x, a[i].y);
  }
}

int main()
{
  srand(time(0));

  RenderWindow window(VideoMode(320, 480), "The Game!", sf::Style::Titlebar || sf::Style::None);

  Texture t1,t2,t3;
  t1.loadFromFile("images/tiles.png");
  t2.loadFromFile("images/background.png");
  t3.loadFromFile("images/frame.png");

  Sprite s(t1), background(t2), frame(t3);
//  Vector2f v = Vector2f(2.0, 2.0);
//  s.scale(v);


  Text debug;
  Font font = Font();
  if (!font.loadFromFile("fonts/arial.ttf")) {
    printf("Font file not found\n");
    exit(0);
  }

  debug.setFont(font);
  debug.setFillColor(Color::Red);
  debug.setCharacterSize(40);
  //debug.setPosition(0.f, 0.f);
  debug.setPosition(15.f, window.getSize().y / 2.0f);


  int dx = 0; bool rotate=0; int colorNum=1;
  float timer=0, delay=0.3;

  Clock clock;

  while (window.isOpen())
  {
    float time = clock.getElapsedTime().asSeconds();
    clock.restart();
    timer += time;

    Event e;
    while (window.pollEvent(e))
    {
      if (e.type == Event::Closed)
        window.close();

      if (e.type == Event::KeyPressed) {
        if (e.key.code  == Keyboard::Up) rotate = true;
        else if (e.key.code == Keyboard::Left) dx = -1;
        else if (e.key.code == Keyboard::Right) dx = 1;
        else if (e.key.code == Keyboard::Escape) exit(0);
      }
    }

    if (Keyboard::isKeyPressed(Keyboard::Down)) delay = 0.05;

    // Move
    for (int i=0;i<4;i++) { b[i] = a[i]; a[i].x+=dx; }
    if (!check()) {
      for (int i=0; i<4; i++) a[i] = b[i];
    }


    // Rotate
    if (rotate) {
      printA();
      Point p = a[1]; // center of rotation
      for (int i=0; i<4; i++) {
        int x = a[i].y-p.y;
        int y = a[i].x-p.x;
        a[i].x = p.x - x;
        a[i].y = p.y + y;
      }
      if (!check()) {
        for (int i=0;i<4;i++) { a[i] = b[i]; }
      }      
    }

    if (timer > delay) {
      printf("t:%f", timer);
      for (int i=0; i<4; i++) { b[i] = a[i]; a[i].y +=1; }

      if (!check()) {
        for (int i=0;i<4;i++) { field[b[i].y][b[i].x] = colorNum; }
        colorNum = 1 + rand()%7;
        int n = rand()%7;
        for (int i=0;i<4;i++) {
          a[i].x = figures[n][i]%2;
          a[i].y = figures[n][i]/2;
        }
      }

      timer = 0;
    }

    // check lines
    int k = M-1;
    for (int i=M-1;i>0;i--)
    {
      int count = 0;
      for (int j=0;j<N;j++)
      {
        if (field[i][j]) count++;
        field[k][j]=field[i][j];
      }
      if (count<N) k--;
    }


    dx = 0; rotate = 0; delay = 0.3;

    // Draw
    window.clear(Color::White);
    //window.draw(background);

    sf::Vertex line[2];
    line[0].position = sf::Vector2f(0, 10);
    line[0].color  = sf::Color::Red;
    line[1].position = sf::Vector2f(M*tileSize, 10);
    line[1].color = sf::Color::Red;
    window.draw(line, 2, Quads);

    sf::VertexArray lines(sf::LinesStrip, M*2);
    for (int i=0;i<M*2;i+=2) {
      lines[i].position = sf::Vector2f(0, tileSize*i);
      lines[i].color  = sf::Color::Black;
      lines[i+1].position = sf::Vector2f(M*tileSize, tileSize*i);
      lines[i+1].color  = sf::Color::Blue;
    }
    window.draw(lines);


    for (int i=0;i<M;i++) {
      for (int j=0;j<N;j++) {
        if (field[i][j]==0) continue;
        s.setTextureRect(IntRect(field[i][j]*tileSize,0,tileSize,tileSize));
        s.setPosition(j*tileSize,i*tileSize);
        s.move(28,31); //offset
        window.draw(s);
      }
    }

    for (int i=0; i<4; i++) {
      s.setTextureRect(IntRect(colorNum*tileSize,0,tileSize,tileSize));
      s.setPosition(a[i].x*tileSize, a[i].y*tileSize);
      s.move(28,31); //offset
      window.draw(s);
    }



    //window.draw(frame);
    debug.setString("DEBUG");
    window.draw(debug);
    window.display();
  }

  return 0;
}
