#include <cmath>
#include <iostream>
#include <raylib.h>
#include <string>
#include <vector>

const int OneUnit = 100;
const int BorderHeight = 720, BorderWidth = 1280;

struct coord {
    float x = BorderWidth / 2.0f;
    float y = BorderHeight / 2.0f;

    friend std::ostream& operator<<(std::ostream& os, const coord& c) {
        os << "(" << c.x << ", " << c.y << ")";
        return os;
    }
};

struct Size {
    float width = 0.0f;
    float height = 0.0f;

    friend std::ostream& operator<<(std::ostream& os, const Size& s) {
        os << s.width << "x" << s.height;
        return os;
    }
};

enum class WoundSeverity {
    Superficial,
    Severe,
    Fatal
};

struct Wound {
    WoundSeverity severity = WoundSeverity::Superficial;

    friend std::ostream& operator<<(std::ostream& os, const Wound& w) {
        switch (w.severity) {
            case WoundSeverity::Superficial: os << "Superficial"; break;
            case WoundSeverity::Severe:      os << "Severe";      break;
            case WoundSeverity::Fatal:       os << "Fatal";       break;
            default:                         os << "Unknown";     break;
        }
        return os;
    }
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

    friend std::ostream& operator<<(std::ostream& os, const HoveredItem& item) {
        os << item.show();
        return os;
    }
};

class Joint {
private:
    coord position{};
    float radius = 10.0f;
    int id = -1;

public:
    Joint() = default;
    Joint(const coord& pos, float radius) : position(pos), radius(radius) {}

    friend std::ostream& operator<<(std::ostream& os, const Joint& j) {
        os << "Joint[" << j.id << "] pos=" << j.position << " r=" << j.radius;
        return os;
    }

    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    coord getPosition() const { return position; }
    void setPosition(const coord& newPos) { position = newPos; }

    float getRadius() const { return radius; }
    void setRadius(float newRadius) { radius = newRadius; }

    bool containsPoint(Vector2 mouse) const {
        return CheckCollisionPointCircle(mouse, Vector2{position.x, position.y}, radius);
    }

    void draw(bool isCoupled, Vector2 mouse) const {
        Color fill = SKYBLUE;
        if (isCoupled) {
            fill = DARKBLUE;
        }
        if (containsPoint(mouse)) {
            fill = GOLD;
        }

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
    float strength = 1.0f;
    float weight = 1.0f;
    float durability = 1.0f;
    int parent = -1;
    float angle = 0.0f;
    Wound wound{};
    bool isDragging = false;
    coord grabOffset{};

public:
    Part() = default;

    Part(const coord& pos, const Size& sz, float str, float wgt, float dur, int parentIdx = -1)
        : position(pos),
          size(sz),
          strength(str),
          weight(wgt),
          durability(dur),
          parent(parentIdx) {}

    friend std::ostream& operator<<(std::ostream& os, const Part& p) {
        os << "Part[" << p.id << "] pos=" << p.position
           << " size=" << p.size
           << " angle=" << p.angle
           << " wound=" << p.wound;
        return os;
    }

    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    coord getPosition() const { return position; }
};

class Limb : public Part {
protected:
    Joint* joints[2] = {nullptr, nullptr};
    int CoupleIndex = -1; // -1 - nelegat, 0 -> joints[0] e parinte, 1 -> joints[1] e parinte

    float boneDensity = 1.0f;
    float muscleDensity = 1.0f;
    float boneLength = 1.0f;

    void updateDimentions() {
        size.width = (boneDensity + muscleDensity);
        size.height = boneLength;
        const float jointRadius = (boneDensity / 10.0f * OneUnit) + 0.75f;
        for (Joint* j : joints) {
            if (j != nullptr) {
                j->setRadius(jointRadius);
            }
        }
    }

public:
    Limb() {
        updateDimentions();
    }

    Limb(const coord& pos, float boneDens, float muscleDens, float length, float initialAngle = 0.0f)
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

    Limb(const Limb& other)
        : Part(other),
          CoupleIndex(other.CoupleIndex),
          boneDensity(other.boneDensity),
          muscleDensity(other.muscleDensity),
          boneLength(other.boneLength) {
        if (other.joints[0] != nullptr) {
            joints[0] = new Joint(*other.joints[0]);
        }
        if (other.joints[1] != nullptr) {
            joints[1] = new Joint(*other.joints[1]);
        }
    }

    ~Limb() {
        delete joints[0];
        delete joints[1];
    }

    Limb& operator=(const Limb& other) {
        if (this == &other) {
            return *this;
        }

        Part::operator=(other);
        CoupleIndex = other.CoupleIndex;
        boneDensity = other.boneDensity;
        muscleDensity = other.muscleDensity;
        boneLength = other.boneLength;

        delete joints[0];
        delete joints[1];
        joints[0] = nullptr;
        joints[1] = nullptr;

        if (other.joints[0] != nullptr) {
            joints[0] = new Joint(*other.joints[0]);
        }
        if (other.joints[1] != nullptr) {
            joints[1] = new Joint(*other.joints[1]);
        }

        return *this;
    }

    friend std::ostream& operator<<(std::ostream& os, const Limb& l) {
        const Part& base = l;
        os << "Limb{" << base
           << ", boneDens=" << l.boneDensity
           << ", muscleDens=" << l.muscleDensity
           << ", length=" << l.boneLength;
        if (l.joints[0] != nullptr) os << ", j0=" << *l.joints[0];
        if (l.joints[1] != nullptr) os << ", j1=" << *l.joints[1];
        os << "}";
        return os;
    }

    void setPosition(const coord& newPos) {
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
        return CheckCollisionPointLine(mouse, Vector2{p0.x, p0.y}, Vector2{p1.x, p1.y}, static_cast<int>(radius));
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
        if (index < 0 || index > 1 || newJoint == nullptr) {
            return;
        }
        delete joints[index];
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

        const float boneThickness = boneDensity / 10.0f * OneUnit;
        const float totalThickness = size.width / 10.0f * OneUnit;

        DrawLineEx(start, end, totalThickness, Fade(MAROON, 0.55f));
        DrawLineEx(start, end, boneThickness, RAYWHITE);
    }
};

class Button {
private:
    Size size{};
    Color color{LIGHTGRAY};
    coord position{};
    bool swich = true;
    bool isPressed = false;

public:
    Button() = default;
    Button(const Size& size, const Color& color, const coord& poz, bool Toggle)
        : size(size), color(color), position(poz), swich(Toggle) {}

    friend std::ostream& operator<<(std::ostream& os, const Button& b) {
        os << "Button pos=" << b.position
           << " size=" << b.size
           << " pressed=" << (b.isPressed ? "true" : "false");
        return os;
    }

    bool getIsPressed() const { return isPressed; }

    bool ContainsPoint(Vector2 mouse) const {
        return CheckCollisionPointRec(mouse, Rectangle{position.x, position.y, size.width, size.height});
    }

    void Toggle(Vector2 mouse) {
        if (swich) {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && ContainsPoint(mouse)) {
                isPressed = !isPressed;
            }
        } else {
            isPressed = IsMouseButtonDown(MOUSE_BUTTON_LEFT) && ContainsPoint(mouse);
        }
    }

    void draw() const {
        const Color fill = isPressed ? DARKGRAY : color;
        DrawRectangleV(Vector2{position.x, position.y}, Vector2{size.width, size.height}, fill);
        DrawRectangleLinesEx(Rectangle{position.x, position.y, size.width, size.height}, 2.0f, BLACK);
    }
};

class Editor {
private:
    std::vector<Limb*> allLimbs;
    std::vector<Joint*> allJoints;

public:
    Editor() = default;

    explicit Editor(int initialLimbCount) {
        for (int i = 0; i < initialLimbCount; ++i) {
            createLimb();
        }
    }

    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    ~Editor() {
        for (Limb* limb : allLimbs) {
            delete limb;
        }
    }

    friend std::ostream& operator<<(std::ostream& os, const Editor& e) {
        os << "Editor (" << e.allLimbs.size() << " limbs, " << e.allJoints.size() << " joints):\n";
        for (const Limb* limb : e.allLimbs) {
            if (limb != nullptr) {
                os << "  " << *limb << "\n";
            }
        }
        return os;
    }

    Limb* createLimb() {
        coord spawnPos{BorderWidth / 3.0f, BorderHeight * 2.0f / 3.0f};
        Limb* limb = new Limb(spawnPos, 1.0f, 1.0f, 1.0f, 0.0f);

        limb->setId(static_cast<int>(allLimbs.size()));
        allLimbs.push_back(limb);

        Joint* j0 = limb->getJoint(0);
        j0->setId(static_cast<int>(allJoints.size()));
        allJoints.push_back(j0);

        Joint* j1 = limb->getJoint(1);
        j1->setId(static_cast<int>(allJoints.size()));
        allJoints.push_back(j1);

        return limb;
    }

    std::vector<HoveredItem> getHoveredItems(Vector2 mouse) const {
        std::vector<HoveredItem> hovered;

        for (const Limb* limb : allLimbs) {
            if (limb->containsPoint(mouse)) {
                hovered.push_back(HoveredItem{HoverType::Limb, limb->getId()});
            }
        }

        for (const Joint* joint : allJoints) {
            if (joint->containsPoint(mouse)) {
                hovered.push_back(HoveredItem{HoverType::Joint, joint->getId()});
            }
        }

        return hovered;
    }

    void drawHoveredItems() const {
        int Y = 20;
        for (const auto& item : getHoveredItems(GetMousePosition())) {
            DrawText(item.show().c_str(), 20, Y, 20, DARKGRAY);
            Y += 20;
        }
    }
};

int main() {
    Editor editor;
    Limb* limb = editor.createLimb();
    Limb* limb2 = editor.createLimb();
    limb2->changeBone(0.2f, 1.2f);
    limb2->changeMuscle(0.3f);
    limb2->realiniere(0.5f);

    Limb previewLimb(coord{100.0f, 100.0f}, 1.0f, 1.0f, 1.0f, 0.0f);
    previewLimb.replaceJoint(1, new Joint(coord{150.0f, 150.0f}, previewLimb.getJoint(0)->getRadius()));
    std::cout << previewLimb << "\n";

    Button spawnBtn(Size{120.0f, 40.0f}, LIGHTGRAY, coord{20.0f, 650.0f}, true);
    std::cout << spawnBtn << " (pressed=" << spawnBtn.getIsPressed() << ")\n";

    int mode = 1;
    std::cin >> mode;
    if (mode == 0) {
        std::cout << editor << "\n";
        return 0;
    }

    limb2->setPosition({BorderWidth / 2.0f, BorderHeight / 2.0f});
    InitWindow(BorderWidth, BorderHeight, "Engene");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        Vector2 mouse = GetMousePosition();
        spawnBtn.Toggle(mouse);
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
        spawnBtn.draw();
        editor.drawHoveredItems();

        EndDrawing();
    }
    std::cout << editor << "\n";
    CloseWindow();
    return 0;
}