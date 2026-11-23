#include <iostream>
#include <ranges>
#include <raylib.h>
#include <cmath>
#include <memory>
#include <vector>
#include<unordered_set>


const int OneUnit=100;
const float BorderHeight=720, BorderWidth=1280;
struct coord {
    float x=BorderWidth/2;
    float y=BorderHeight/2;
};
struct Size {
    float width=0.;
    float height=0.;
};
enum class WoundSeverity {
    Superficial,
    Severe,
    Fatal
};
struct Wound {
    WoundSeverity severity;
};

enum class HoverType {
    Limb,
    Joint
};

struct HoveredItem {
    HoverType type;
    int id;

    std::string show() const {
        std::string item;
        switch (type) {
            case HoverType::Limb:  item = "Limb";    break;
            case HoverType::Joint: item = "Joint";   break;
            default:               item = "Unknown"; break;
        }
        return item + " [" + std::to_string(id) + "]";
    }
};






class Joint {
private:
    coord position;
    float radius;
    int id = -1;
    public:
    int getId() const { return id; }
    void setId(int newId) { id = newId; }
    Joint()=default;
    Joint(coord pos, float radius): position(pos), radius(radius) {}

    coord getPosition() const {return position;}
    void setPosition(coord newPos) {position = newPos;}
    float getRadius() const {return radius;}
    void setRadius(float newRadius) {radius = newRadius;}

    bool containsPoint(Vector2 mouse) const {return CheckCollisionPointCircle(mouse, Vector2{position.x,position.y},radius);}

    void draw(bool isCoupled,Vector2 mouse) const {
        Color fill=SKYBLUE;
        if (isCoupled) {
            fill=DARKBLUE;}
        if (containsPoint(mouse)) {fill=GOLD;}

        const Vector2 center{position.x, position.y};
        DrawCircleV(center, radius, fill);
        DrawCircleLinesV(center, radius, BLACK);
    }

};

class Part {
protected:
    int id = -1;
    coord position{};
    Size size{};
    float strength = 1.;
    float weight = 1.;
    float durability = 1.;
    int parent = -1;
    float angle = 0.;
    Wound wound{};
    bool isDragging = false;
    coord grabOffset{};

public:
    Part() = default;

    Part(coord pos, Size sz, float str, float wgt, float dur, int parentIdx = -1)
        : position(pos),
          size(sz),
          strength(str),
          weight(wgt),
          durability(dur),
          parent(parentIdx) {}

    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    coord getPosition() const { return position; }
    void setPosition(coord newPos) { position = newPos; }

    bool containsPoint(Vector2 mouse) const {
        const Rectangle bounds{
            position.x - size.width * 0.5f,
            position.y - size.height * 0.5f,
            size.width,
            size.height
        };
        return CheckCollisionPointRec(mouse, bounds);
    }

    bool IsHeld(Vector2 mouse) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && containsPoint(mouse)) {
            isDragging = true;
            grabOffset = coord{position.x - mouse.x, position.y - mouse.y};
        }

        if (isDragging) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                setPosition(coord{mouse.x + grabOffset.x, mouse.y + grabOffset.y});
                return true;
            }
            isDragging = false;
        }
        return false;
    }
};





class Limb : public Part {
protected:
    Joint* joints[2] = {nullptr, nullptr};
    int CoupleIndex = -1; // -1 - nelegat, 0 -> joints[0] e parinte, 1 -> joints[1] e parinte

    float boneDensity = 1.;
    float muscleDensity = 1.;
    float boneLength = 1.;

    void updateDimentions() {
        size.width = (boneDensity + muscleDensity);
        size.height = boneLength;
        const float jointRadius = (boneDensity / 10. * OneUnit) + 0.75;
        for (Joint* j : joints) {
            if (j) j->setRadius(jointRadius);
        }
    }

public:
    Limb() {
        updateDimentions();
    }

    Limb(coord pos, float boneDens, float muscleDens, float length, float initialAngle = 0.0f)
        : Part(pos, Size{}, 1.0f, 1.0f, 1.0f),
          boneDensity(boneDens),
          muscleDensity(muscleDens),
          boneLength(length) {
        angle = initialAngle;
        const float pixelLength = boneLength * OneUnit;
        const coord endPos{
            pos.x + std::sin(angle) * pixelLength,
            pos.y + std::cos(angle) * pixelLength
        };

        joints[0] = new Joint(pos, 10.0f);
        joints[1] = new Joint(endPos, 10.0f);

        updateDimentions();
    }

    ~Limb() {
        delete joints[0];
        delete joints[1];
    }

    void setPosition(coord newPos) {
        position = newPos;
        const float pixelLength = boneLength * OneUnit;
        joints[0]->setPosition(position);
        joints[1]->setPosition(coord{
            position.x + std::sin(angle) * pixelLength,
            position.y + std::cos(angle) * pixelLength
        });
    }

    bool containsPoint(Vector2 mouse) const {
        const coord p0 = joints[0]->getPosition();
        const coord p1 = joints[1]->getPosition();
        const float radius = (size.width / 10.0f * OneUnit) * 0.5f;
        return CheckCollisionPointLine(mouse, Vector2{p0.x, p0.y}, Vector2{p1.x, p1.y}, radius);
    }

    bool IsHeld(Vector2 mouse) {
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && containsPoint(mouse)) {
            isDragging = true;
            grabOffset = coord{position.x - mouse.x, position.y - mouse.y};
        }

        if (isDragging) {
            if (IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
                setPosition(coord{mouse.x + grabOffset.x, mouse.y + grabOffset.y});
                return true;
            }
            isDragging = false;
        }
        return false;
    }

    Joint* getJoint(int index) const { return joints[index]; }

    void replaceJoint(int index, Joint* newJoint) {
        joints[index] = newJoint;
        updateDimentions();
    }

    void changeBone(float density, float lenght) {
        boneDensity += density;
        boneLength = lenght;
        updateDimentions();
    }

    void changeMuscle(float desity) {
        muscleDensity += desity;
        updateDimentions();
    }

    void realiniere(float newAngle) {
        angle = newAngle;
        setPosition(position);
    }

    void draw() const {
        const coord p0 = joints[0]->getPosition();
        const coord p1 = joints[1]->getPosition();
        const Vector2 start{p0.x, p0.y};
        const Vector2 end{p1.x, p1.y};

        const float boneThickness = boneDensity / 10 * OneUnit;
        const float totalThickness = size.width / 10 * OneUnit;

        DrawLineEx(start, end, totalThickness, Fade(MAROON, 0.55f));
        DrawLineEx(start, end, boneThickness, RAYWHITE);
    }
};




// Non class stuff

std::vector<Limb*> allLimbs;
std::vector<Joint*> allJoints;
Part* selectedPart = nullptr;


Limb* createLimb() {
    coord spawnPos{BorderWidth / 3.0f, BorderHeight * 2.0f / 3.0f};
    Limb* limb = new Limb(spawnPos, 1.0f, 1.0f, 1.0f, 0.0f);

    limb->setId(allLimbs.size());
    allLimbs.push_back(limb);

    Joint* j0 = limb->getJoint(0);
    j0->setId(allJoints.size());
    allJoints.push_back(j0);

    Joint* j1 = limb->getJoint(1);
    j1->setId(allJoints.size());
    allJoints.push_back(j1);

    return limb;
}

std::vector<HoveredItem> getHoveredItems(Vector2 mouse) {
    std::vector<HoveredItem> hovered;

    for (Limb* limb : allLimbs) {
        if (limb->containsPoint(mouse)) {
            hovered.push_back(HoveredItem{HoverType::Limb, limb->getId()});
        }
    }

    for (Joint* joint : allJoints) {
        if (joint->containsPoint(mouse)) {
            hovered.push_back(HoveredItem{HoverType::Joint, joint->getId()});
        }
    }

    return hovered;
}
void drawHoveredItems() {
    int Y = 20;
    for (auto& item : getHoveredItems(GetMousePosition())) {
        DrawText(item.show().c_str(), 20, Y, 20, DARKGRAY);
        Y+=20;
    }
}


int main() {
    Limb *limb=createLimb();
    Limb *limb2=createLimb();
    limb2->setPosition({BorderWidth/2,BorderHeight/2});
    InitWindow(BorderWidth, BorderHeight, "Engene");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        limb->IsHeld(mouse);
        limb2->IsHeld(mouse);


        BeginDrawing();
        ClearBackground(RAYWHITE);
        limb->draw();
        limb2->draw();
        limb->getJoint(0)->draw(false, mouse);
        limb->getJoint(1)->draw(false, mouse);
        limb2->getJoint(0)->draw(false, mouse);
        limb2->getJoint(1)->draw(false, mouse);
        // for (auto& i:getHoveredItems(mouse)) {
        //     std::cout << i.show() << std::endl;
        // }
        // EndDrawing();
        drawHoveredItems();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}