#include "imgui.h" // necessary for ImGui::*, imgui-SFML.h doesn't include imgui.h
#include "imgui-SFML.h" // for ImGui::SFML::* functions and SFML-specific overloads


#include <Box2D/Box2D.h>
#include <SFML/Graphics.hpp>

#define DEGTORAD 0.0174532925199432957f
#define RADTODEG 57.295779513082320876f

const int W = 1024;
const int H = 768;

const float PPM = 32.0f;

b2Vec2 gravity(0.0f, 0.3f);
b2World world(gravity);
bool paused = false;

using namespace sf;

struct Entity
{
    const char* name;
    float width, height;
    float xinit, yinit;

    b2BodyDef def;
    b2PolygonShape shape;
    b2FixtureDef fixture;
    b2Body* body;

    sf::RectangleShape sfShape;

    Entity(const char* n, float w, float h, float xc, float yc, Color color) {
        name = n;
        width = w; height = h;
        xinit = xc; yinit = yc;

        def.userData.pointer = reinterpret_cast<uintptr_t>(this);
        def.position.Set(xinit / PPM, yinit / PPM);
        shape.SetAsBox(1.05f * width * .5f / PPM, height * .5f / PPM);
        // 1.05 factor because testing platforms showed it was needed
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

    Entity* next;
};

struct EntityList {
    Entity* head;
    Entity* tail;
    EntityList& add(Entity* node) {
        if (head == 0) {
            head = tail = node;
        }
        else {
            tail->next = node;
            node->next = 0;
            tail = node;
        }
        return *this;
    }
};

struct DynamicEntity : Entity {

    float density, friction;

    DynamicEntity(const char* name, float w, float h, float xc, float yc, float d, float f, Color color) :
        Entity(name, w, h, xc, yc, color)
    {
        density = d; friction = f;
        def.type = b2_dynamicBody;
        def.angle = rand() % 360 * DEGTORAD;

        shape.SetAsBox(width * .5f / PPM, height * .5f / PPM);

        fixture.density = density;
        fixture.friction = friction;
    }

    void build() {
        Entity::build();
        sfShape.setRotation(body->GetAngle() * RADTODEG);
    }

    void update() {
        if (body->GetPosition().x >= W / PPM)
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

class MyContactListener : public b2ContactListener
{
    void BeginContact(b2Contact* contact) {

        Entity* exit = 0;
        Entity* player = 0;
        b2BodyUserData data = contact->GetFixtureA()->GetBody()->GetUserData();
        if (data.pointer != 0) {
            Entity* b = (Entity*)data.pointer;
            if (strcmp(b->name, "player") == 0) {
                player = b;
            }
            else if (strcmp(b->name, "exit") == 0) {
                exit = b;
            }
        }

        data = contact->GetFixtureB()->GetBody()->GetUserData();
        if (data.pointer != 0) {
            Entity* b = (Entity*)data.pointer;
            if (strcmp(b->name, "player") == 0) {
                player = b;
            }
            else if (strcmp(b->name, "exit") == 0) {
                exit = b;
            }
        }
        if (exit && player) {
            exit->sfShape.setFillColor(Color::Black);
        }
    }

    void EndContact(b2Contact* contact) {

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

struct Player {

    DynamicEntity entity;
    b2Body* body;

    bool jumpButtomPressed, jumpButtomReleased;
    bool jumping;
    int jumpCount = 0;
    float posY, prevPosY;

    Player() : entity("player", W / 40, W / 40, W / 10, 0.0f, 0.3f, 0.5f, Color::Green) {
        entity.build();
        body = entity.body;
    }
    void keyboardJump() {
        if (jumpCount < 2) {
            body->SetLinearVelocity(b2Vec2(body->GetLinearVelocity().x, -1.35f));
            jumpCount++;
        }
    }
    void moveRight() {
        body->SetLinearVelocity(b2Vec2(0.3f, body->GetLinearVelocity().y));
        //body->SetAngularVelocity(20 * DEGTORAD);

    }
    void moveLeft() {
        body->SetLinearVelocity(b2Vec2(-0.3f, body->GetLinearVelocity().y));
        //body->SetAngularVelocity(-20 * DEGTORAD);
    }

    void update() {

        if (jx > 40) moveRight();
        if (jx < -40) moveLeft();

        posY = body->GetPosition().y * PPM;

        float lv = round(PPM * body->GetLinearVelocity().y * 1000.0) / 1000.0;
        if (lv == 0.0f && abs(posY - prevPosY) == 0.0f)
            jumpCount = 0; jumping = false;

        prevPosY = body->GetPosition().y * PPM;
        entity.update();
    }

    void recreate() {
        entity.recreate();
    }

    const Joystick::Axis padAxisX = static_cast<Joystick::Axis>(0);
    const Joystick::Axis padAxisY = static_cast<Joystick::Axis>(1);
    const int padButtonA = 0;
    int jx, jy;

    void joystick(int id) {

        jx = Joystick::getAxisPosition(id, padAxisX);
        jy = Joystick::getAxisPosition(id, padAxisY);

        if (Joystick::isButtonPressed(id, padButtonA)) {

            if (jumpButtomReleased) {
                if (jumpCount < 2) {
                    body->SetLinearVelocity(b2Vec2(body->GetLinearVelocity().x, -1.35f));
                    jumping = true; jumpCount++;
                    jumpButtomReleased = false;
                }
            }

        }
        else if (!Joystick::isButtonPressed(id, padButtonA)) {
            jumpButtomReleased = true;
        }
    }
};

int main() {
    srand(time(0));
    RenderWindow app(VideoMode(W, H, 32), "Box2D", Style::Titlebar);
    ImGui::SFML::Init(app);
    Grid grid;

    MyContactListener myContactListener;
    world.SetContactListener(&myContactListener);

    float wallWidth = W / 80;
    Entity leftWall("lw", wallWidth, H / 1.3f, 0.0f + wallWidth * .5f, H / 2, Color::White); leftWall.build();
    Entity rightWall("rw", wallWidth, H / 1.3f, W - wallWidth * .5f, H / 2, Color::White); rightWall.build();
    Entity ground("ground", W, wallWidth, W / 2, H - wallWidth * .5f, Color::White); ground.build();
    Entity p1("p1", W / 10, wallWidth, 2 * W / 10, 9 * H / 10, Color::White); p1.build();
    Entity p2("p2", W / 10, wallWidth, 4 * W / 10, 8 * H / 10, Color::White); p2.build();
    Entity p3("p3", W / 10, wallWidth, 6 * W / 10, 7 * H / 10, Color::White); p3.build();
    Entity exit("exit", W / 30, W / 20, W / 3, 9.4 * H / 10, Color::Green); exit.build();

    Player player;
    DynamicEntity redBox("redBox", W / 20, W / 20, W / 4, 0.0f, 1.0f, 0.1f, Color::Red); redBox.build();
    DynamicEntity blueBox("blueBox", W / 20, W / 20, 3 * W / 4, 0.0f, 5.0f, 5.0f, Color::Blue); blueBox.build();

    EntityList entityList;
    entityList.add(&leftWall).add(&rightWall).add(&ground).add(&p1).add(&p2).add(&p3).add(&exit);
    entityList.add(&redBox).add(&blueBox).add(&player.entity);

    for (Entity* e = entityList.head; e != 0; e = e->next) {

        printf("e:%s | ", e->name);
    }

    //text stuff to appear on the page
    Font myFont;
    if (!myFont.loadFromFile("sansation.ttf")) { return 1; }

    Text pausedText("PAUSED", myFont);
    pausedText.setCharacterSize(20);
    pausedText.setColor(Color::Red);
    pausedText.setPosition(25, 25);

    Text exitInstructions("[Escape] to exit", myFont);
    Text moveInstructions("[WASD] to move square", myFont);
    exitInstructions.setCharacterSize(18);
    moveInstructions.setCharacterSize(18);
    exitInstructions.setColor(Color::Blue);
    moveInstructions.setColor(Color::Blue);
    exitInstructions.setPosition(25, 50);
    moveInstructions.setPosition(25, 70);

    float freq = 130.0f;
    float oneStepFreq = 2000.0f;

    float timeStep = 1 / freq;
    int32 velocityIterations = 6;
    int32 positionIterations = 2;

    sf::Clock deltaClock;
    bool show_imgui_demo = true;

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
                    player.keyboardJump();
                }

                if (e.key.code == Keyboard::R) {
                    player.recreate();
                }

                if (e.key.code == Keyboard::Escape)
                    app.close();

                if (e.key.code == Keyboard::G)
                    grid.isVisible = !grid.isVisible;

                if (e.key.code == Keyboard::P) {
                    paused = !paused;
                    timeStep = paused ? 0.0f : 1 / freq;
                }

                if (e.key.code == Keyboard::O) {
                    timeStep = 1 / oneStepFreq;
                    paused = false;
                }
                if (e.key.code == Keyboard::Comma) {
                    freq -= 30;
                    timeStep = 1 / freq;
                }
                if (e.key.code == Keyboard::Period) {
                    timeStep = 1 / freq;
                    freq += 30;
                }

            }
            if (e.type == Event::KeyReleased) {
                if (e.key.code == Keyboard::O) {
                    paused = true;
                    timeStep = 0.0f;
                }
            }

            if ((e.type == Event::JoystickButtonPressed) ||
                (e.type == Event::JoystickButtonReleased) ||
                (e.type == Event::JoystickMoved) ||
                (e.type == Event::JoystickConnected)) {

                // Update displayed joystick values
                int id = e.joystickConnect.joystickId;
                player.joystick(id);
            }
        } // pollEvent

        if (Keyboard::isKeyPressed(sf::Keyboard::A)) {
            player.moveLeft();
        }
        if (Keyboard::isKeyPressed(sf::Keyboard::D)) {
            player.moveRight();
        }


        ImGui::SFML::Update(app, deltaClock.restart());

        if (show_imgui_demo)
            ImGui::ShowDemoWindow(&show_imgui_demo);
        {
            ImGui::Begin("myBox");
            float f1 = 0.0f;

            //ImGui::LabelText("timeStep", "%.6f %.f", timeStep);
            ImGui::SliderFloat("timeStep freq", &freq, 100.0f, 4000.0f, "%f");
            ImGui::SliderFloat("oneStep freq", &oneStepFreq, 2000.0f, 4000.0f, "%f");

            ImGui::LabelText("posY - prevPosy", "%.6f %.6f", player.posY, player.prevPosY);

            b2Vec2 pos = player.body->GetPosition();
            ImGui::LabelText("Position", "(%f, %f)", PPM * pos.x, PPM * pos.y);

            b2Vec2 lv = player.body->GetLinearVelocity();
            ImGui::LabelText("LinearVelocity", "(%f, %f)", PPM * lv.x, PPM * lv.y);
            float av = player.body->GetAngularVelocity();
            ImGui::LabelText("AngularVelocity", "%f", PPM * av);

            ImGui::LabelText("jumpCount", "%i", player.jumpCount);

            ImGui::End();
        }

        player.update();

        redBox.update();
        blueBox.update();


        app.clear();

        if (paused) {
            app.draw(pausedText);
        }

        app.draw(exitInstructions);
        app.draw(moveInstructions);

        for (Entity* e = entityList.head; e != 0; e = e->next) {
            app.draw(e->sfShape);
        }

        grid.draw(app);
        ImGui::SFML::Render(app);
        app.display();

    } //app.isOpen()

    ImGui::SFML::Shutdown();
    return 0;
}