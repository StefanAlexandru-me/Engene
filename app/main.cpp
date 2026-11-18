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

public:
    Part()=default;

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

    void setParent(Part* p){parent=p;}
    void setChild(Part* c){child=c;}
    Part* getParent(){return parent;}
    Part* getChild(){return child;}

    void changeBone(float density,float lenght) {
        boneDensity=density;
        boneLength=lenght;
        updateDimentions();
    }
    void changeMuscle(float desity) {
        muscleDensity=desity;
        updateDimentions();
    }
    coord getEndPosition() const {
        const float pixelLength = boneLength * OneUnit;
        return coord{
            position.x + std::sin(angle) * pixelLength,
            position.y + std::cos(angle) * pixelLength
        };
    }

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
    InitWindow(BorderWidth, BorderHeight, "Engene");
    SetTargetFPS(60);
Limb limb;
    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(RAYWHITE);
        DrawText("Engene - Raylib activ", 20, 20, 20, DARKGRAY);
        limb.draw();
        EndDrawing();
    }

    CloseWindow();
    return 0;
}