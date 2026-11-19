#include <iostream>
#include <ranges>
#include <raylib.h>
#include <cmath>


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

class Part {
protected:
    coord position{};
    Size size{};
    float strength=1.;
    float weight=1.;
    float durability=1.;
    int parent=-1;
    float angle=0.;
    Wound wound;
    bool isDragging = false;
    coord grabOffset;

public:
    Part()=default;
    Part(coord pos, Size sz, float str, float wgt, float dur, int parentIdx = -1)
        : position(pos),
          size(sz),
          strength(str),
          weight(wgt),
          durability(dur),
          parent(parentIdx) {}
    coord getPosition() const {return position;}

    virtual void setPosition(coord newPos) {
        position = newPos;
    }

    //to be checked

    virtual bool containsPoint(Vector2 mouse) const {
        const Rectangle bounds{
            position.x - size.width * 0.5f,
            position.y - size.height * 0.5f,
            size.width,
            size.height
        };
        return CheckCollisionPointRec(mouse, bounds);
    }

    //
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




class Limb:public Part {
protected:
    Part* parent=nullptr;
    Part* child=nullptr;

    float boneDensity=1.;
    float muscleDensity=1.;
    float boneLength=1.;
    void updateDimentions()
    {size.width=(boneDensity+muscleDensity);
    size.height=boneLength;}

public:
    Limb() {
        updateDimentions();
    }
    Limb(coord pos, float boneDens, float muscleDens, float length)
        : Part(pos, Size{}, 1.0f, 1.0f, 1.0f),
          boneDensity(boneDens),
          muscleDensity(muscleDens),
          boneLength(length) {
        updateDimentions();
    }

void setParent(Part* p) { parent = p; }
    void setChild(Part* c) {
        child = c;
        if (child != nullptr) {
            child->setPosition(getEndPosition());
        }
    }
    Part* getParent(){return parent;}
    Part* getChild(){return child;}



    void changeBone(float density,float lenght) {
        boneDensity+=density;
        boneLength=lenght;
        updateDimentions();
    }
    void changeMuscle(float desity) {
        muscleDensity+=desity;
        updateDimentions();
    }
    coord getEndPosition() const {
        const float pixelLength = boneLength * OneUnit;
        return coord{
            position.x + std::sin(angle) * pixelLength,
            position.y + std::cos(angle) * pixelLength
        };
    }
    void setPosition(coord newPos) override {
        position = newPos;
        if (child != nullptr) {
            child->setPosition(getEndPosition());
        }
    }

    void realiniere(float newAngle) {
        angle = newAngle;
        setPosition(position);
    }
    //test this

    bool containsPoint(Vector2 mouse) const override {
        const coord endPos = getEndPosition();
        const Vector2 start{position.x, position.y};
        const Vector2 end{endPos.x, endPos.y};
        const float radius = (size.width/10 * OneUnit) * 0.5f;

        return CheckCollisionPointLine(mouse, start, end, static_cast<int>(radius)) ||
               CheckCollisionPointCircle(mouse, start, radius) ||
               CheckCollisionPointCircle(mouse, end, radius);
    }

    //

    void draw() const {
        const coord endPos = getEndPosition();
        const Vector2 start{position.x, position.y};
        const Vector2 end{endPos.x, endPos.y};

        const float boneThickness = boneDensity/10 * OneUnit;
        const float totalThickness = size.width/10 * OneUnit;


        DrawLineEx(start, end, totalThickness, Fade(MAROON, 0.55f));

        DrawLineEx(start, end, boneThickness, RAYWHITE);

        const float jointRadius = boneThickness * 1.2f;
        DrawCircleV(start, jointRadius, DARKBLUE);
        DrawCircleV(end, jointRadius, SKYBLUE);
    }


};


int main() {
   Limb limb;
    Limb limb2;

    InitWindow(BorderWidth, BorderHeight, "Engene");
    SetTargetFPS(60);
    while (!WindowShouldClose()) {
        limb.IsHeld(GetMousePosition());

        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Engene - Raylib activ", 20, 20, 20, DARKGRAY);
        limb.draw();
        limb2.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}