#include <cmath>
#include <functional>
#include <iostream>
#include <raylib.h>
#include <string>
#include <utility>
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
    float getAngle() const { return angle; }
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
        if (joints[0] != nullptr && joints[1] != nullptr) {
            setPosition(position);
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
        if (CoupleIndex != 0) {
            delete joints[0];
        }
        if (CoupleIndex != 1) {
            delete joints[1];
        }
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

    bool isCoupled() const { return CoupleIndex != -1; }
    int getCoupleIndex() const { return CoupleIndex; }
    bool getIsDragging() const { return isDragging; }

    void setPosition(const coord& newPos) {
        position = newPos;
        const float pixelLength = boneLength * OneUnit;
        if (CoupleIndex == 1) {
            const coord parentPos = joints[1]->getPosition();
            position = coord{
                parentPos.x - std::sin(angle) * pixelLength,
                parentPos.y - std::cos(angle) * pixelLength
            };
            joints[0]->setPosition(position);
        } else {
            joints[0]->setPosition(position);
            joints[1]->setPosition(coord{
                position.x + std::sin(angle) * pixelLength,
                position.y + std::cos(angle) * pixelLength
            });
        }
    }

    void syncWithParent() {
        if (CoupleIndex != -1 && joints[CoupleIndex] != nullptr) {
            setPosition(joints[CoupleIndex]->getPosition());
        }
    }

    void setParentJoint(int index, Joint* parentJoint) {
        if (index < 0 || index > 1 || parentJoint == nullptr) {
            return;
        }
        joints[index] = parentJoint;
        CoupleIndex = index;

        const coord parentPos = parentJoint->getPosition();
        const float pixelLength = boneLength * OneUnit;
        if (index == 0) {
            setPosition(parentPos);
        } else {
            setPosition(coord{
                parentPos.x - std::sin(angle) * pixelLength,
                parentPos.y - std::cos(angle) * pixelLength
            });
        }
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

    void changeLength(float deltaLength) {
        boneLength += deltaLength;
        if (boneLength < 0.2f) {
            boneLength = 0.2f;
        }
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
    int id = -1;
    Size size{};
    Color color{LIGHTGRAY};
    coord position{};
    bool swich = false;
    bool isPressed = false;
    std::function<void()> onClick{};

public:
    Button() = default;
    Button(const Size& size, const Color& color, const coord& poz, bool toggle, std::function<void()> action = nullptr)
        : size(size),
          color(color),
          position(poz),
          swich(toggle),
          onClick(std::move(action)) {}

    friend std::ostream& operator<<(std::ostream& os, const Button& b) {
        os << "Button[" << b.getId() << "] pos=" << b.position
           << " size=" << b.size
           << " pressed=" << (b.getIsPressed() ? "true" : "false");
        return os;
    }

    int getId() const { return id; }
    void setId(int newId) { id = newId; }

    bool getIsPressed() const { return isPressed; }

    bool ContainsPoint(Vector2 mouse) const {
        return CheckCollisionPointRec(mouse, Rectangle{position.x, position.y, size.width, size.height});
    }

    void update(Vector2 mouse) {
        const bool hovered = ContainsPoint(mouse);
        if (swich) {
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                isPressed = !isPressed;
            }
            if (isPressed && onClick) {
                onClick();
            }
        } else {
            if (hovered && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
                isPressed = true;
                if (onClick) {
                    onClick();
                }
            } else if (!IsMouseButtonDown(MOUSE_BUTTON_LEFT) || !hovered) {
                isPressed = false;
            }
        }
    }

    void draw() const {
        const Color fill = isPressed ? DARKGRAY : color;
        DrawRectangleV(Vector2{position.x, position.y}, Vector2{size.width, size.height}, fill);
        DrawRectangleLinesEx(Rectangle{position.x, position.y, size.width, size.height}, 2., BLACK);
    }
};

class Editor {
private:
    std::vector<Limb*> allLimbs;
    std::vector<Joint*> allJoints;
    std::vector<Button> buttons;

    HoverType selectedType = HoverType::Limb;
    int selectedId = -1;

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
        os << "Editor (" << e.allLimbs.size() << " limbs, " << e.allJoints.size() << " joints, "
           << e.buttons.size() << " buttons):\n";
        for (const Limb* limb : e.allLimbs) {
            if (limb != nullptr) {
                os << "  " << *limb << "\n";
            }
        }
        for (const Button& btn : e.buttons) {
            os << "  " << btn << "\n";
        }
        if (e.selectedId != -1) {
            os << "  Selected: " << HoveredItem{e.selectedType, e.selectedId} << "\n";
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

    void combineJoints(int parentJointId, int childJointId) {
        if (parentJointId == childJointId) {
            return;
        }

        Joint* parentJoint = nullptr;
        Limb* parentLimb = nullptr;
        int parentSlot = -1;

        Joint* childJoint = nullptr;
        Limb* childLimb = nullptr;
        int childSlot = -1;

        for (Limb* limb : allLimbs) {
            for (int i = 0; i < 2; ++i) {
                if (limb->getCoupleIndex() == i) {
                    continue;
                }
                Joint* j = limb->getJoint(i);
                if (j == nullptr) {
                    continue;
                }
                if (j->getId() == parentJointId) {
                    parentJoint = j;
                    parentLimb = limb;
                    parentSlot = i;
                }
                if (j->getId() == childJointId) {
                    childJoint = j;
                    childLimb = limb;
                    childSlot = i;
                }
            }
        }

        if (parentJoint == nullptr || childJoint == nullptr || parentLimb == nullptr || childLimb == nullptr ||
            parentLimb == childLimb || parentSlot == -1 || childSlot == -1) {
            return;
        }

        if (childLimb->isCoupled() && !parentLimb->isCoupled()) {
            std::swap(parentJoint, childJoint);
            std::swap(parentLimb, childLimb);
            std::swap(parentSlot, childSlot);
        }

        if (childLimb->isCoupled()) {
            return;
        }


        const int removedJointId = childJoint->getId();
        for (auto it = allJoints.begin(); it != allJoints.end(); ++it) {
            if (*it == childJoint) {
                allJoints.erase(it);
                break;
            }
        }
        delete childJoint;

        if (selectedType == HoverType::Joint && selectedId == removedJointId) {
            selectedId = -1;
        }

        childLimb->setParentJoint(childSlot, parentJoint);
    }

    void combineJoints() {
        std::vector<int> hoveredJointIds;
        for (const HoveredItem& item : getHoveredItems(GetMousePosition())) {
            if (item.type == HoverType::Joint) {
                hoveredJointIds.push_back(item.id);
            }
        }

        if (hoveredJointIds.size() >= 2) {
            combineJoints(hoveredJointIds[0], hoveredJointIds[1]);
        }
    }

    void createButton(const Size& size, const Color& color, const coord& pos, bool toggle, std::function<void()> action = nullptr) {
        Button btn(size, color, pos, toggle, std::move(action));
        btn.setId(static_cast<int>(buttons.size()));
        buttons.push_back(std::move(btn));
    }

    Limb* getSelectedLimb() const {
        if (selectedType != HoverType::Limb || selectedId < 0 || selectedId >= static_cast<int>(allLimbs.size())) {
            return nullptr;
        }
        return allLimbs[selectedId];
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

    void updateSelection(Vector2 mouse) {
        if (!IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            return;
        }

        const std::vector<HoveredItem> hovered = getHoveredItems(mouse);
        if (hovered.empty()) {
            selectedId = -1;
            return;
        }

        selectedType = hovered.back().type;
        selectedId = hovered.back().id;
    }

    void update(Vector2 mouse) {
        bool overButton = false;
        for (Button& btn : buttons) {
            btn.update(mouse);
            if (btn.ContainsPoint(mouse)) {
                overButton = true;
            }
        }

        if (IsKeyPressed(KEY_C)) {
            combineJoints();
        }

        if (overButton && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            return;
        }

        if (!overButton) {
            updateSelection(mouse);
        }

        for (Limb* limb : allLimbs) {
            if (!limb->isCoupled() && limb->IsHeld(mouse)) {
                break;
            }
        }

        for (int i = 0; i < static_cast<int>(allLimbs.size()); i++) {
            for (Limb* limb : allLimbs) {
                limb->syncWithParent();
            }
        }
    }

    void drawSelectedItem() const {
        if (selectedId == -1) {
            return;
        }
        const std::string text = "Selected: " + HoveredItem{selectedType, selectedId}.show();
        DrawText(text.c_str(), 170, 20, 20, GREEN);
    }

    void drawHoveredItems() const {
        int Y = 20;
        for (const auto& item : getHoveredItems(GetMousePosition())) {
            DrawText(item.show().c_str(), 70, Y, 20, DARKGRAY);
            Y += 20;
        }
    }

    void draw(Vector2 mouse) const {
        for (const Limb* limb : allLimbs) {
            limb->draw();
        }
        for (const Joint* joint : allJoints) {
            joint->draw(false, mouse);
        }
        for (const Button& btn : buttons) {
            btn.draw();
        }
        drawHoveredItems();
        drawSelectedItem();
    }
};

void LoadScenario(Editor& editor) {
    // buton creere limb
    editor.createButton(Size{80., 40.}, LIGHTGRAY, coord{20., 650.}, false, [&editor]() {
        editor.createLimb();
    });

    // buton adaugare lungime
    editor.createButton(Size{80., 40.}, SKYBLUE, coord{110., 650.}, true, [&editor]() {
        if (Limb* selected = editor.getSelectedLimb()) {
            selected->changeLength(0.2);
        }
    });

    // scadere lungime
    editor.createButton(Size{80., 40.}, RED, coord{200., 650.}, true, [&editor]() {
        if (Limb* selected = editor.getSelectedLimb()) {
            selected->changeLength(-0.02);
        }
    });

    // scimbare unghi+
    editor.createButton(Size{80., 40.}, YELLOW, coord{290., 650.}, true, [&editor]() {
        if (Limb* selected = editor.getSelectedLimb()) {
            selected->realiniere(selected->getAngle() + 0.1);
        }
    });

    // scimbare unghi-
    editor.createButton(Size{80., 40.}, GREEN, coord{380., 650.}, true, [&editor]() {
        if (Limb* selected = editor.getSelectedLimb()) {
            selected->realiniere(selected->getAngle() - 0.1);
        }
    });
}

int main() {
    Editor editor;
    LoadScenario(editor);

    int mode = 1;
    std::cin >> mode;
    if (mode == 0) {
        Limb previewLimb(coord{100.0f, 100.0f}, 1.0f, 1.0f, 1.0f, 0.0f);
        previewLimb.changeBone(0.1f, 1.0f);
        previewLimb.changeMuscle(0.1f);
        previewLimb.realiniere(0.0f);
        previewLimb.replaceJoint(1, new Joint(coord{150.0f, 150.0f}, previewLimb.getJoint(0)->getRadius()));
        std::cout << editor << "\n" << previewLimb << "\n";
        return 0;
    }

    InitWindow(BorderWidth, BorderHeight, "Engene");
    SetTargetFPS(60);

    while (!WindowShouldClose()) {
        const Vector2 mouse = GetMousePosition();
        editor.update(mouse);

        BeginDrawing();
        ClearBackground(RAYWHITE);
        editor.draw(mouse);
        EndDrawing();
    }

    std::cout << editor << "\n";
    CloseWindow();
    return 0;
}