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
    b2FixtureDef fixtureDef;
    b2Body* body;

    RectangleShape sfShape;

    Entity(const char* _name, float _width, float _height, float _xinit, float _yinit, Color color) {
        name = _name;
        width = _width; height = _height;
        xinit = _xinit; yinit = _yinit;

        def.userData.pointer = reinterpret_cast<uintptr_t>(this);
        def.position.Set(xinit / PPM, yinit / PPM);
        shape.SetAsBox(width * .5f / PPM, height * .5f / PPM);
        //shape.SetAsBox(1.05f * width * .5f / PPM, height * .5f / PPM);
        // 1.05 factor because testing platforms showed it was needed
        fixtureDef.shape = &shape;

        sfShape.setSize(Vector2f(width, height));
        sfShape.setOrigin(width * .5f, height * .5f);
        sfShape.setFillColor(color);
        sfShape.setPosition(xinit, yinit);
    }


    virtual void build() {
        body = world.CreateBody(&def);
        body->CreateFixture(&fixtureDef);
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
            tail = node;
        }
        node->next = 0;
        return *this;
    }

    void draw(RenderWindow& window) {
        for (Entity* e = head; e != 0; e = e->next) {
            window.draw(e->sfShape);
        }
    }
};

struct DynamicEntity : Entity {

    float density, friction;

    DynamicEntity(const char* _name, float _width, float _height, float _xinit, float _yinit,
                  float _density, float _friction, Color color) :
        Entity(_name, _width, _height, _xinit, _yinit, color)
    {
        density = _density; friction = _friction;
        def.type = b2_dynamicBody;
        def.angle = rand() % 360 * DEGTORAD;

        shape.SetAsBox(width * .5f / PPM, height * .5f / PPM);

        fixtureDef.density = density;
        fixtureDef.friction = friction;
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
        body->CreateFixture(&fixtureDef);
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

struct MyText {
    MyText(Text* t) { text = t; }
    Text* text;
    MyText* next;
};

struct HelpTexts {
    Font& font;
    MyText* head;
    MyText* tail;
    HelpTexts(Font& myFont) : font(myFont) { }

    HelpTexts& add(const char* s, float x, float y, float size = 18.0f, Color color = Color::Blue) {
        Text* t = new Text(s, font, size);
        t->setPosition(x, y);
        t->setColor(color);

        MyText* myText = new MyText(t);
        if (head == 0) {
            head = tail = myText;
        }
        else {
            tail->next = myText;
            tail = myText;
        }
        myText->next = 0;

        return *this;
    }
    void draw(RenderWindow& window) {
        for (MyText* t = head; t != 0; t = t->next) {
            window.draw(*t->text);
        }
    }
};

struct Player : public b2ContactListener{

    DynamicEntity entity;
    b2Body* body;
    RectangleShape foot;

    bool jumpButtomPressed, jumpButtomReleased;
    int jumpCount = 0;

    Player() : entity("player", W / 40, W / 40, W / 10, 0.0f, 0.3f, 0.3f, Color::Green) {
        
        //entity.fixtureDef.friction = 0.0f;
        entity.fixtureDef.restitution = 0.3f;

        entity.build();
        body = entity.body;
        
        // // Foot Sensor
        // float footWidth = entity.width * .5f;
        // float footHeight = entity.height * .5f;
        // entity.shape.SetAsBox(footWidth*.5f / PPM, footHeight*.5f / PPM, b2Vec2(0, -footHeight/PPM), 0);
        // entity.fixtureDef.isSensor = true;
        // entity.fixtureDef.userData.pointer = reinterpret_cast<uintptr_t>(this);
        // b2Fixture* footSensor = body->CreateFixture(&entity.fixtureDef);

        // foot.setSize(Vector2f(footWidth, footHeight));
        // foot.setOrigin(footWidth/2, footHeight);
        // foot.setFillColor(Color::Red);
        // foot.setPosition(entity.xinit, entity.yinit);
    }
    void keyboardJump() {
        if (jumpCount < 2) {
            b2Vec2 vel = body->GetLinearVelocity(); vel.y = -1.35;
            body->SetLinearVelocity(vel);
            jumpCount++;
            entity.fixtureDef.friction = 0.0f;
        }
    }
    void moveRight() {
        b2Vec2 vel = body->GetLinearVelocity(); vel.x = 0.3f;
        body->SetLinearVelocity(vel);
        body->SetAngularVelocity(20 * DEGTORAD);

    }
    void moveLeft() {
        b2Vec2 vel = body->GetLinearVelocity(); vel.x = -0.3f;
        body->SetLinearVelocity(vel);
        body->SetAngularVelocity(-20 * DEGTORAD);
    }

    void update() {

        if (jx > 40) moveRight();
        if (jx < -40) moveLeft();

        entity.update();

        //foot.setRotation(body->GetAngle() * RADTODEG);
        //foot.setPosition(body->GetPosition().x * PPM, body->GetPosition().y * PPM);        
    }

    void draw(RenderWindow& window) {
        window.draw(foot);
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

            entity.fixtureDef.friction = 0.3f;

            if (jumpButtomReleased) {
                if (jumpCount < 2) {
                    body->SetLinearVelocity(b2Vec2(body->GetLinearVelocity().x, -1.35f));
                    jumpCount++;
                    jumpButtomReleased = false;
                }
            }

        }
        else if (!Joystick::isButtonPressed(id, padButtonA)) {
            jumpButtomReleased = true;
        }
    }

    void BeginContact(b2Contact* contact) {

        // Check body contact with exit
        Entity* exit = 0;
        Entity* player = 0;
        Entity* leftWall = 0;
        Entity* rightWall = 0;

        b2BodyUserData data = contact->GetFixtureA()->GetBody()->GetUserData();
        if (data.pointer != 0) {
            Entity* b = (Entity*)data.pointer;
            if (strcmp(b->name, "player") == 0) {
                player = b;
            }
            else if (strcmp(b->name, "exit") == 0) {
                exit = b;
            }
            else if (strcmp(b->name, "leftWall") == 0) {
                leftWall = b;
            }
            else if (strcmp(b->name, "rightWall") == 0) {
                rightWall = b;
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
            else if (strcmp(b->name, "leftWall") == 0) {
                leftWall = b;
            }
            else if (strcmp(b->name, "rightWall") == 0) {
                rightWall = b;
            }            
        }
        if (exit && player) {
            exit->sfShape.setFillColor(Color::Black);
        }

        if (player && leftWall) {
            float impulse = body->GetMass();
            body->ApplyLinearImpulse( b2Vec2(impulse, 0), body->GetWorldCenter(), true);      
            moveRight();
            body->ApplyLinearImpulse( b2Vec2(0, -impulse*0.01), body->GetWorldCenter(), true);
            return;
        }
        if (player && rightWall) {
            
            float impulse = body->GetMass();
            body->ApplyLinearImpulse( b2Vec2(-2*impulse, -impulse*0.01), body->GetWorldCenter(), true);             
            //moveLeft();
            return;
        }
        if (player) {
            jumpCount = 0;
        }

    }

    void EndContact(b2Contact* contact) {
        // Check foot sensor

    }    
};

int main() {
    srand(time(0));
    RenderWindow app(VideoMode(W, H, 32), "Box2D", Style::Titlebar);
    ImGui::SFML::Init(app);
    Grid grid;

    float wallThickness = W / 80;
    Entity leftWall("leftWall", wallThickness, H / 1.3f, 0.0f + wallThickness * .5f, H / 2, Color::White); leftWall.build();
    Entity rightWall("rightWall", wallThickness, H / 1.3f, W - wallThickness * .5f, H / 2, Color::White); rightWall.build();
    Entity ground("ground", W, wallThickness, W / 2, H - wallThickness * .5f, Color::White); ground.build();
    Entity p1("p1", W / 10, wallThickness, 2*W/10, 9 * H / 10, Color::White); p1.build();
    Entity p2("p2", W / 10, wallThickness, 4*W/10, 8 * H / 10, Color::White); p2.build();
    Entity p3("p3", W / 10, wallThickness, 6*W/10, 7 * H / 10, Color::White); p3.build();
    Entity exit("exit", W / 30, W / 20, 6*W/10, 6.4 * H / 10, Color::Green); exit.build();

    Player player;
    world.SetContactListener(&player);

    DynamicEntity redBox("redBox", W / 20, W / 20, W / 4, 0.0f, 1.0f, 0.1f, Color::Red); redBox.build();
    DynamicEntity blueBox("blueBox", W / 20, W / 20, 3 * W / 4, 0.0f, 5.0f, 5.0f, Color::Blue); blueBox.build();

    EntityList entityList;
    entityList.add(&leftWall).add(&rightWall).add(&ground).add(&p1).add(&p2).add(&p3).add(&exit);
    entityList.add(&redBox).add(&blueBox).add(&player.entity);

    // for (Entity* e = entityList.head; e != 0; e = e->next) {
    //     printf("e:%s | ", e->name);
    // }

    //text stuff to appear on the page
    Font myFont;
    if (!myFont.loadFromFile("sansation.ttf")) { return 1; }
    HelpTexts helpTexts(myFont);
    helpTexts.add("[Escape] to exit", W / 32, H / 24);
    helpTexts.add("[WASD] to move square", W / 32, 2 * H / 24);
    helpTexts.add("[P] - Pause   [O] - Single step", W / 32, 3 * H / 24);

    HelpTexts pausedText(myFont);
    pausedText.add("PAUSED", W / 2, H / 2, 30, Color::Red);

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

            b2Vec2 pos = player.body->GetPosition();
            ImGui::LabelText("Position", "(%f, %f)", PPM * pos.x, PPM * pos.y);

            b2Vec2 lv = player.body->GetLinearVelocity();
            ImGui::LabelText("LinearVelocity", "(%f, %f)", PPM * lv.x, PPM * lv.y);
            float av = player.body->GetAngularVelocity();
            ImGui::LabelText("AngularVelocity", "%f", PPM * av);

            ImGui::LabelText("jumpCount", "%i", player.jumpCount);

            ImGui::End();
        }
        app.clear();

        player.update();
        redBox.update();
        blueBox.update();

        if (paused) {
            pausedText.draw(app);
        }
        helpTexts.draw(app);
        entityList.draw(app);
        player.draw(app);

        grid.draw(app);
        ImGui::SFML::Render(app);
        app.display();

    } //app.isOpen()

    ImGui::SFML::Shutdown();
    return 0;
}