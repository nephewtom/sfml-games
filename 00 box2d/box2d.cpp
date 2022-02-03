#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f

const int W = 800;
const int H = 600;

const float PPM = 32.0f;

b2Vec2 gravity(0.0f, 0.3f);
b2World world(gravity);

using namespace sf;

struct Body
{
    float width, height;
    float xinit, yinit;

    b2BodyDef def;
    b2PolygonShape shape;
    b2FixtureDef fixture;
    b2Body* body;
    sf::RectangleShape sfShape;

    Body(float w, float h, float xc, float yc, Color color) {
        width = w; height = h;
        xinit = xc; yinit = yc;

        def.position.Set(xinit / PPM, yinit / PPM);

        shape.SetAsBox(1.05f * width * .5f / PPM, height * .5f / PPM);
        fixture.shape = &shape;

        sfShape.setSize(Vector2f(width, height));
        sfShape.setOrigin(width * .5f, height * .5f);
        sfShape.setFillColor(color);
        sfShape.setPosition(xinit, yinit);
    }

    virtual void build() {
        body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
    }
};

struct DynamicBody : Body {

    float density, friction;

    DynamicBody(float w, float h, float xc, float yc, float d, float f, Color color) :
        Body(w, h, xc, yc, color)
    {
        density = d; friction = f;
        def.type = b2_dynamicBody;
        def.angle = rand() % 360 * DEGTORAD;

        shape.SetAsBox(width * .5f / PPM, height * .5f / PPM);

        fixture.density = density;
        fixture.friction = friction;
    }

    void build() {
        Body::build();
        sfShape.setRotation(body->GetAngle() * RADTODEG);
    }

    void update() {
        if (body->GetPosition().x > W / PPM)
            body->SetTransform(b2Vec2(0.0f, body->GetPosition().y), body->GetAngle());
        if (body->GetPosition().x < 0) 
            body->SetTransform(b2Vec2(W / PPM, body->GetPosition().y), body->GetAngle());

        sfShape.setRotation(body->GetAngle() * RADTODEG);
        sfShape.setPosition(body->GetPosition().x * PPM, body->GetPosition().y * PPM);
    }

    void recreate() {
        body->DestroyFixture(body->GetFixtureList());
        world.DestroyBody(body);
        def.position.Set(xinit / PPM, yinit / PPM);
        def.angle = rand() % 360 * DEGTORAD;
        body = world.CreateBody(&def);
        body->CreateFixture(&fixture);
    }
};

class Grid {
public:
    const static int space = 50;
    const static int hNum = H / space;
    const static int wNum = W / space;

    RectangleShape hLines[hNum + 1];
    RectangleShape wLines[wNum + 1];

    bool isVisible = false;

    Grid() {
        for (int i = 0; i <= hNum; i++) {
            hLines[i].setSize(Vector2f(W, 1));
            hLines[i].setPosition(0, space * i);
        }
        hLines[hNum].setPosition(0, H - 1);

        for (int i = 0; i <= wNum; i++) {
            wLines[i].setSize(Vector2f(1, H));
            wLines[i].setPosition(space * i, 0);
        }
        wLines[wNum].setPosition(W - 1, 0);
    }

    void draw(RenderWindow& window) {
        if (isVisible) {
            for (int i = 0; i <= hNum; i++) {
                window.draw(hLines[i]);
            }
            for (int i = 0; i <= wNum; i++) {
                window.draw(wLines[i]);
            }
        }
    }
};

int main() {
    srand(time(0));
    RenderWindow app(VideoMode(W, H, 32), "Box2D sample", Style::Titlebar);

    Grid grid;

    float wallWidth = 10.0f;
    Body leftWall(wallWidth, H/1.3f, 0.0f+wallWidth*.5f, H/2, Color::White); leftWall.build();
    Body rightWall(wallWidth, H/1.3f, W-wallWidth*.5f, H/2, Color::White); rightWall.build();
    Body ground(W, wallWidth, W/2, H-wallWidth*.5f, Color::White); ground.build();

    DynamicBody myBox(20.0f, 20.0f, 300.0f, 0.0f, 0.3f, 0.5f, Color::Green); myBox.build();
    DynamicBody oBox(40.0f, 40.0f, 500.0f, 0.0f, 1.0f, 0.1f, Color::Red); oBox.build();
    DynamicBody pBox(40.0f, 40.0f, 200.0f, 0.0f, 1.0f, 0.1f, Color::Blue); pBox.build();


    //text stuff to appear on the page
    Font myFont;
    if (!myFont.loadFromFile("sansation.ttf")) { return 1; }

    Text text("FPS", myFont);
    text.setCharacterSize(20);
    text.setColor(Color(0, 255, 255, 255));
    text.setPosition(25, 25);

    Text clearInstructions("Press [Escape] to exit", myFont);
    Text jumpInstructions("[WASD] to move square", myFont);
    clearInstructions.setCharacterSize(18);
    jumpInstructions.setCharacterSize(18);
    clearInstructions.setColor(Color(200, 55, 100, 255));
    jumpInstructions.setColor(Color(200, 55, 100, 255));
    clearInstructions.setPosition(25, 50);
    jumpInstructions.setPosition(25, 70);

    float timeStep = 1 / 130.0f;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;
    int jumpCount = 0;

    while (app.isOpen())
    {
        world.Step(timeStep, velocityIterations, positionIterations);

        Event e;
        while (app.pollEvent(e)) {

            if (e.type == Event::Closed)
                app.close();

            if (e.type == Event::KeyPressed) {

                if (e.key.code == Keyboard::W) {
                    if (jumpCount < 2) {
                        myBox.body->SetLinearVelocity(b2Vec2(myBox.body->GetLinearVelocity().x, -1.35f));
                        jumpCount++;
                    }
                }

                if (e.key.code == Keyboard::R) {
                    myBox.recreate();
                }

                if (e.key.code == Keyboard::Escape)
                    app.close();

                if (e.key.code == Keyboard::G)
                    grid.isVisible = !grid.isVisible;

            }
            if (e.type == Event::KeyReleased) {
                if (e.key.code == Keyboard::W) {
                }
            }

            if ((e.type == Event::JoystickButtonPressed) ||
                (e.type == Event::JoystickButtonReleased) ||
                (e.type == Event::JoystickMoved) ||
                (e.type == Event::JoystickConnected)) {

                // Update displayed joystick values
                int id = e.joystickConnect.joystickId;
                //players[id]->joystick(id);
            }
        }
        if (Keyboard::isKeyPressed(sf::Keyboard::A)) {
            myBox.body->SetLinearVelocity(b2Vec2(-0.3f, myBox.body->GetLinearVelocity().y));
            myBox.body->SetAngularVelocity(-20 * DEGTORAD);

            if (jumpCount != 0) {
                //myBox.body->SetAngularVelocity(-45 * DEGTORAD);
            }
        }
        if (Keyboard::isKeyPressed(sf::Keyboard::D)) {
            myBox.body->SetLinearVelocity(b2Vec2(0.3f, myBox.body->GetLinearVelocity().y));
            myBox.body->SetAngularVelocity(20 * DEGTORAD);

            if (jumpCount != 0) {
                //myBox.body->SetAngularVelocity(45 * DEGTORAD);
            }
        }
        if (myBox.body->GetLinearVelocity().y == 0.0f) {
            jumpCount = 0;
        }

        myBox.update();
        oBox.update();
        pBox.update();


        app.clear();

        app.draw(text);
        app.draw(clearInstructions);
        app.draw(jumpInstructions);

        app.draw(leftWall.sfShape);
        app.draw(rightWall.sfShape);
        app.draw(ground.sfShape);
        app.draw(oBox.sfShape);
        app.draw(pBox.sfShape);
        app.draw(myBox.sfShape);

        grid.draw(app);

        app.display();
    }

    return 0;
}