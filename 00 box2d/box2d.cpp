#include "imgui.h" // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h
#include "imgui-SFML.h" // for ImGui::SFML::* functions and SFML-specific overloads


#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f

const int W = 800;
const int H = 600;

const float PPM = 32.0f;
struct Body
{
    float width, height;
    Body(float w, float h) {
        width = w;
        height = h;
    }
    b2BodyDef def;
    b2PolygonShape shape;
    b2FixtureDef fixture;
    b2Body* body;
    sf::RectangleShape sfShape;
};
b2Vec2 gravity(0.0f, 0.3f);
b2World world(gravity);

using namespace sf;

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
    RenderWindow app(VideoMode(W, H, 32), "Box2D", Style::Titlebar);
    ImGui::SFML::Init(app);

    Grid grid;

    // ground - static body
    float gX = 400.0f, gY = 580.0f;
    b2BodyDef groundDef;
    groundDef.position.Set(gX / PPM, gY / PPM);
    b2Body* groundBox = world.CreateBody(&groundDef);

    float gW = 600.0f, gH = 25.0f;
    b2PolygonShape groundShape;
    groundShape.SetAsBox(1.05f * gW * 0.5f / PPM, gH * 0.5f / PPM);
    b2FixtureDef groundFixture;
    groundFixture.shape = &groundShape;
    groundBox->CreateFixture(&groundShape, 0.0f);

    RectangleShape sfGroundShape(Vector2f(gW, gH));
    sfGroundShape.setOrigin(gW / 2.0f, gH / 2.0f);
    sfGroundShape.setFillColor(Color::Blue);
    sfGroundShape.setPosition(gX, gY);


    // dynamic body
    b2BodyDef boxDef;
    boxDef.type = b2_dynamicBody;
    boxDef.position.Set(300.0f / PPM, 0.0f / PPM);
    boxDef.angle = rand() % 360 * DEGTORAD;
    b2Body* boxBody = world.CreateBody(&boxDef);

    b2PolygonShape boxShape;
    boxShape.SetAsBox(10.0f / PPM, 10.0f / PPM);
    b2FixtureDef fixtureDef;
    fixtureDef.shape = &boxShape;
    fixtureDef.density = 0.3f;
    fixtureDef.friction = 0.5f;
    boxBody->CreateFixture(&fixtureDef);

    RectangleShape sfBoxShape(Vector2f(20, 20));
    sfBoxShape.setOrigin(10, 10);
    sfBoxShape.setFillColor(Color::Green);
    sfBoxShape.setRotation(boxBody->GetAngle() * RADTODEG);


    Body oBox(40.0f, 40.0f);
    oBox.def.type = b2_dynamicBody;
    oBox.def.position.Set(500.0f / PPM, 0.0f / PPM);
    oBox.def.angle = rand() % 360 * DEGTORAD;
    oBox.body = world.CreateBody(&oBox.def);
    oBox.shape.SetAsBox(oBox.width * .5f / PPM, oBox.height * .5f / PPM);
    oBox.fixture.shape = &oBox.shape;
    oBox.fixture.density = 1.0f;
    oBox.fixture.friction = 0.1f;
    oBox.body->CreateFixture(&oBox.fixture);
    oBox.sfShape.setSize(Vector2f(oBox.width, oBox.height));
    oBox.sfShape.setOrigin(oBox.width * .5f, oBox.height * .5f);
    oBox.sfShape.setFillColor(Color::Red);
    oBox.sfShape.setRotation(oBox.body->GetAngle() * RADTODEG);


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

    float timeStep = 1 / 180.0f;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;
    bool keyPressed = false;
    int jumpCount = 0;

    sf::Clock deltaClock;

    while (app.isOpen())
    {
        world.Step(timeStep, velocityIterations, positionIterations);

        Event e;
        while (app.pollEvent(e)) {

            ImGui::SFML::ProcessEvent(app, e);

            if (e.type == Event::Closed)
                app.close();

            if (e.type == Event::KeyPressed) {

                if (e.key.code == Keyboard::W) {
                    if (jumpCount < 2) {
                        boxBody->SetLinearVelocity(b2Vec2(boxBody->GetLinearVelocity().x, -1.35f));
                        jumpCount++;
                    }
                }

                if (e.key.code == Keyboard::R) {
                    boxBody->DestroyFixture(boxBody->GetFixtureList());
                    world.DestroyBody(boxBody);
                    boxDef.position.Set(400.0f / PPM, 0.0f / PPM);
                    boxBody = world.CreateBody(&boxDef);
                    boxDef.angle = rand() % 360 * DEGTORAD;
                    boxBody->CreateFixture(&fixtureDef);


                    oBox.body->DestroyFixture(oBox.body->GetFixtureList());
                    world.DestroyBody(oBox.body);
                    oBox.def.position.Set(500.0f / PPM, 0.0f / PPM);
                    oBox.def.angle = rand() % 360 * DEGTORAD;
                    oBox.body = world.CreateBody(&oBox.def);
                    oBox.body->CreateFixture(&oBox.fixture);

                }

                if (e.key.code == Keyboard::Escape)
                    app.close();

                if (e.key.code == Keyboard::G)
                    grid.isVisible = !grid.isVisible;

            }
            if (e.type == Event::KeyReleased) {
                if (e.key.code == Keyboard::W) {
                    //keyPressed = false;
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
            boxBody->SetLinearVelocity(b2Vec2(-0.3f, boxBody->GetLinearVelocity().y));
            if (jumpCount != 0) {
                boxBody->SetAngularVelocity(-45 * DEGTORAD);
            }
            else if (boxBody->GetLinearVelocity().x != 0.0f) {
                boxBody->SetAngularVelocity(-20 * DEGTORAD);
            }
            else {
                boxBody->SetAngularVelocity(0);
            }



        }
        if (Keyboard::isKeyPressed(sf::Keyboard::D)) {
            boxBody->SetLinearVelocity(b2Vec2(0.3f, boxBody->GetLinearVelocity().y));
            if (jumpCount != 0) {
                boxBody->SetAngularVelocity(45 * DEGTORAD);
            }
            else if (boxBody->GetLinearVelocity().x != 0.0f) {
                boxBody->SetAngularVelocity(20 * DEGTORAD);
            }
            else {
                boxBody->SetAngularVelocity(0);
            }
        }

        if (boxBody->GetLinearVelocity().y == 0.0f) {
            jumpCount = 0;
        }

        ImGui::SFML::Update(app, deltaClock.restart());

        ImGui::Begin("Hello, world!");
        ImGui::Button("Look at this pretty button");
        ImGui::End();        


        sfBoxShape.setRotation(boxBody->GetAngle() * RADTODEG);
        sfBoxShape.setPosition(boxBody->GetPosition().x * PPM, boxBody->GetPosition().y * PPM);
        oBox.sfShape.setRotation(oBox.body->GetAngle() * RADTODEG);
        oBox.sfShape.setPosition(oBox.body->GetPosition().x * PPM, oBox.body->GetPosition().y * PPM);

        app.clear();

        app.draw(text);
        app.draw(clearInstructions);
        app.draw(jumpInstructions);
        app.draw(sfGroundShape);
        app.draw(oBox.sfShape);
        app.draw(sfBoxShape);

        grid.draw(app);
        ImGui::SFML::Render(app);
        app.display();
    }

    ImGui::SFML::Shutdown();
    return 0;
}