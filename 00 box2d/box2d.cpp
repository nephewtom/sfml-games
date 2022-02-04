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

using namespace sf;

struct Body
{
    const char* name;
    float width, height;
    float xinit, yinit;

    b2BodyDef def;
    b2PolygonShape shape;
    b2FixtureDef fixture;
    b2Body* body;
    sf::RectangleShape sfShape;

    Body(const char* n, float w, float h, float xc, float yc, Color color) {
        name = n;
        width = w; height = h;
        xinit = xc; yinit = yc;

        def.userData.pointer = reinterpret_cast<uintptr_t>(this);
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

    DynamicBody(const char* name, float w, float h, float xc, float yc, float d, float f, Color color) :
        Body(name, w, h, xc, yc, color)
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

class MyContactListener : public b2ContactListener
{
    void BeginContact(b2Contact* contact) {

        Body* exit = 0;
        Body* player = 0;
        b2BodyUserData data = contact->GetFixtureA()->GetBody()->GetUserData();
        if (data.pointer != 0) {
            Body* b = (Body*)data.pointer;
            if (strcmp(b->name, "player") == 0) {
                player = b;
            }
            else if (strcmp(b->name, "exit") == 0){
                exit = b;
            }
        }

        data = contact->GetFixtureB()->GetBody()->GetUserData();
        if (data.pointer != 0) {
            Body* b = (Body*)data.pointer;
            if (strcmp(b->name, "player") == 0) {
                player = b;
            }
            else if (strcmp(b->name, "exit") == 0){
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

int main() {
    srand(time(0));
    RenderWindow app(VideoMode(W, H, 32), "Box2D", Style::Titlebar);
    ImGui::SFML::Init(app);
    Grid grid;

    MyContactListener myContactListener;
    world.SetContactListener(&myContactListener);

    float wallWidth = W / 80;
    Body leftWall("lw", wallWidth, H / 1.3f, 0.0f + wallWidth * .5f, H / 2, Color::White); leftWall.build();
    Body rightWall("rw", wallWidth, H / 1.3f, W - wallWidth * .5f, H / 2, Color::White); rightWall.build();
    Body ground("ground", W, wallWidth, W / 2, H - wallWidth * .5f, Color::White); ground.build();
    Body platform1("p1", W / 10, wallWidth, 2 * W / 10, 9 * H / 10, Color::White); platform1.build();
    Body platform2("p2", W / 10, wallWidth, 4 * W / 10, 8 * H / 10, Color::White); platform2.build();
    Body platform3("p3", W / 10, wallWidth, 6 * W / 10, 7 * H / 10, Color::White); platform3.build();
    //    Body exit(W/30, W/20, 6*W/10, 6.5*H/10, Color::Green); exit.build();
    Body exit("exit", W / 30, W / 20, W / 3, 9.4 * H / 10, Color::Green); exit.build();



    DynamicBody player("player", W / 40, W / 40, W / 10, 0.0f, 0.3f, 0.5f, Color::Green); player.build();
    DynamicBody redBox("redBox", W / 20, W / 20, W / 4, 0.0f, 1.0f, 0.1f, Color::Red); redBox.build();
    DynamicBody blueBox("blueBox", W / 20, W / 20, 3 * W / 4, 0.0f, 5.0f, 5.0f, Color::Blue); blueBox.build();


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
                    if (jumpCount < 2) {
                        player.body->SetLinearVelocity(b2Vec2(player.body->GetLinearVelocity().x, -1.35f));
                        jumpCount++;
                    }
                }

                if (e.key.code == Keyboard::R) {
                    player.recreate();
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
        } // pollEvent

        if (Keyboard::isKeyPressed(sf::Keyboard::A)) {
            player.body->SetLinearVelocity(b2Vec2(-0.3f, player.body->GetLinearVelocity().y));
            player.body->SetAngularVelocity(-20 * DEGTORAD);

            if (jumpCount != 0) {
                //myBox.body->SetAngularVelocity(-45 * DEGTORAD);
            }
        }
        if (Keyboard::isKeyPressed(sf::Keyboard::D)) {
            player.body->SetLinearVelocity(b2Vec2(0.3f, player.body->GetLinearVelocity().y));
            player.body->SetAngularVelocity(20 * DEGTORAD);

            if (jumpCount != 0) {
                //myBox.body->SetAngularVelocity(45 * DEGTORAD);
            }
        }

        if (abs(player.body->GetLinearVelocity().y * PPM) == 0.0f &&
            abs(player.body->GetAngularVelocity() * PPM) == 0.0f) {
            jumpCount = 0;
        }

        ImGui::SFML::Update(app, deltaClock.restart());

        if (show_imgui_demo)
            ImGui::ShowDemoWindow(&show_imgui_demo);
        {
            ImGui::Begin("myBox");
            float f1 = 0.0f;
            //ImGui::SliderFloat("slider float", &f1, 0.0f, 1.0f, "ratio = %.3f");
            b2Vec2 lv = player.body->GetLinearVelocity();
            float av = player.body->GetAngularVelocity();

            ImGui::LabelText("LinearVelocity", "(%.3f, %.3f)", PPM * lv.x, PPM * lv.y);
            ImGui::LabelText("AngularVelocity", "%.3f", PPM * av);
            Body* b = (Body*)player.def.userData.pointer;
            ImGui::LabelText("jumpCount", "%s %i", b->name, jumpCount);
            //ImGui::SliderFloat("LinearVelocity", &myBox.body->GetLinearVelocity().x, -1.0f, 1.0f, "ratio = %.3f");        

            //ImGui::SliderFloat2("LinearVelocity", myBox.body->GetLinearVelocity(), -1.0, 1.0);        
            ImGui::Button("Look at this pretty button");
            ImGui::End();
        }

        player.update();
        redBox.update();
        blueBox.update();


        app.clear();

        app.draw(text);
        app.draw(clearInstructions);
        app.draw(jumpInstructions);

        app.draw(leftWall.sfShape);
        app.draw(rightWall.sfShape);
        app.draw(ground.sfShape);
        app.draw(platform1.sfShape);
        app.draw(platform2.sfShape);
        app.draw(platform3.sfShape);
        app.draw(exit.sfShape);

        app.draw(redBox.sfShape);
        app.draw(blueBox.sfShape);
        app.draw(player.sfShape);

        grid.draw(app);
        ImGui::SFML::Render(app);
        app.display();

    } //app.isOpen()

    ImGui::SFML::Shutdown();
    return 0;
}