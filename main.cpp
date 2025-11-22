#include <GL/glut.h>
#include <iostream>
#include <vector>
#include <string>
#include <fstream>

struct Task {
    std::string text;
    bool done;
    std::string deadline;
};

std::vector<Task> tasks;
std::string currentInput = "";
int windowWidth = 800, windowHeight = 600;

int scrollOffset = 0;
const int taskHeight = 40;
int draggingTaskIndex = -1;

// Edit mode
bool editing = false;
int editIndex = -1;

// File handling
std::string fileName = "";
bool filenameInputMode = true; // true হলে filename ইনপুট নিচ্ছি

// Save button dimensions
float saveBtnX = 460, saveBtnY = 230, saveBtnW = 80, saveBtnH = 30;

// ================= File I/O ===================
void saveTasksToFile() {
    if (fileName.empty()) return;
    std::ofstream ofs(fileName);
    for (size_t i = 0; i < tasks.size(); ++i) {
        auto &task = tasks[i];
        ofs << (i + 1) << ". " << task.text << "~" << task.deadline << "~" << (task.done ? "Done" : "Undone") << "\n";
    }
}

void loadTasksFromFile() {
    if (fileName.empty()) return;
    std::ifstream ifs(fileName);
    if (!ifs.is_open()) return;

    tasks.clear();
    std::string line;
    while (std::getline(ifs, line)) {
        // Remove order number at start
        size_t dotPos = line.find(". ");
        if(dotPos != std::string::npos)
            line = line.substr(dotPos + 2);

        size_t firstTilde = line.find('~');
        size_t secondTilde = line.find('~', firstTilde + 1);
        if (firstTilde != std::string::npos && secondTilde != std::string::npos) {
            std::string text = line.substr(0, firstTilde);
            std::string deadline = line.substr(firstTilde + 1, secondTilde - firstTilde - 1);
            bool done = (line.substr(secondTilde + 1) == "Done");
            tasks.push_back({text, done, deadline});
        }
    }
}

// ================= Drawing ===================
void drawText(float x, float y, const std::string& text, float r = 1, float g = 1, float b = 1, void* font = GLUT_BITMAP_HELVETICA_18) {
    glColor3f(r, g, b);
    glRasterPos2f(x, y);
    for (char c : text)
        glutBitmapCharacter(font, c);
}

// Draw text centrally inside a box
void drawCenteredText(float boxX, float boxY, float boxW, float boxH, const std::string& text, void* font = GLUT_BITMAP_HELVETICA_18, float r=1, float g=1, float b=1) {
    int textWidth = 0;
    for (char c : text)
        textWidth += glutBitmapWidth(font, c);

    float textX = boxX + (boxW - textWidth) / 2.0f;
    float textY = boxY + (boxH - 18) / 2.0f; // 18 approx font height

    drawText(textX, textY, text, r, g, b, font);
}

void drawButton(float x, float y, float w, float h, const std::string& label, float r=0.2f, float g=0.2f, float b=0.6f) {
    glColor3f(r, g, b);
    glBegin(GL_QUADS);
    glVertex2f(x, y);
    glVertex2f(x + w, y);
    glVertex2f(x + w, y + h);
    glVertex2f(x, y + h);
    glEnd();
    drawCenteredText(x, y, w, h, label);
}

void swapTasks(int i, int j) {
    Task temp = tasks[i];
    tasks[i] = tasks[j];
    tasks[j] = temp;
}

void display() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (filenameInputMode) {
        drawCenteredText(200, windowHeight - 100, 400, 30, "Enter filename and click Save:", GLUT_BITMAP_TIMES_ROMAN_24);
        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(250, windowHeight - 150);
        glVertex2f(550, windowHeight - 150);
        glVertex2f(550, windowHeight - 120);
        glVertex2f(250, windowHeight - 120);
        glEnd();
        drawCenteredText(250, windowHeight - 150, 300, 30, currentInput);
        drawButton(saveBtnX, saveBtnY, saveBtnW, saveBtnH, "Save");
    } else {
        drawText(350, windowHeight - 40, "To-Do", 1.0f, 1.0f, 0.0f, GLUT_BITMAP_TIMES_ROMAN_24);

        float y = windowHeight - 80 + scrollOffset;

        for (size_t i = 0; i < tasks.size(); ++i) {
            auto& task = tasks[i];
            glColor3f(0.2f, 0.2f, 0.2f);
            glBegin(GL_QUADS);
            glVertex2f(50, y - 5);
            glVertex2f(600, y - 5);
            glVertex2f(600, y + 30);
            glVertex2f(50, y + 30);
            glEnd();

            std::string numberedText = std::to_string(i + 1) + ". " + task.text;
            if (task.done)
                drawText(60, y + 10, numberedText, 0.4f, 1.0f, 0.4f);
            else
                drawText(60, y + 10, numberedText);
            drawText(60, y - 5, "Deadline: " + task.deadline, 0.8f, 0.8f, 0.8f);

            drawButton(610, y, 60, 25, "Done");
            drawButton(680, y, 70, 25, "Delete");
            drawButton(760, y, 60, 25, "Edit");

            y -= taskHeight;
        }

        glColor3f(0.3f, 0.3f, 0.3f);
        glBegin(GL_QUADS);
        glVertex2f(50, 20);
        glVertex2f(600, 20);
        glVertex2f(600, 50);
        glVertex2f(50, 50);
        glEnd();

        std::string prefix = editing ? "Edit: " : "Input: ";
        drawCenteredText(50, 20, 550, 30, prefix + currentInput);

        drawButton(610, 20, 110, 30, editing ? "Save" : "+ Add");
    }

    glutSwapBuffers();
}

// ================= Event Handling ===================
void reshape(int w, int h) {
    windowWidth = w;
    windowHeight = h;
    glViewport(0, 0, w, h);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0, w, 0, h);
    glMatrixMode(GL_MODELVIEW);
}

void mouse(int button, int state, int x, int y) {
    y = windowHeight - y;

    if (filenameInputMode) {
        if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
            if (x >= saveBtnX && x <= saveBtnX + saveBtnW && y >= saveBtnY && y <= saveBtnY + saveBtnH) {
                if (!currentInput.empty()) {
                    fileName = currentInput;
                    std::ofstream ofs(fileName, std::ios::app);
                    ofs.close();
                    loadTasksFromFile();
                    currentInput = "";
                    filenameInputMode = false;
                }
            }
        }
        glutPostRedisplay();
        return;
    }

    if (button == GLUT_LEFT_BUTTON) {
        if (x >= 610 && x <= 720 && y >= 20 && y <= 50 && state == GLUT_DOWN) {
            if (!currentInput.empty()) {
                size_t tildePos = currentInput.find('~');
                std::string taskText = currentInput;
                std::string deadline = "No Deadline";

                if (tildePos != std::string::npos) {
                    taskText = currentInput.substr(0, tildePos);
                    deadline = currentInput.substr(tildePos + 1);
                }

                if (editing && editIndex >= 0 && editIndex < (int)tasks.size()) {
                    tasks[editIndex].text = taskText;
                    tasks[editIndex].deadline = deadline;
                    editing = false;
                    editIndex = -1;
                } else {
                    tasks.push_back({taskText, false, deadline});
                }
                currentInput = "";
                saveTasksToFile();
            }
        }

        int ty = windowHeight - 80 + scrollOffset;
        for (size_t i = 0; i < tasks.size(); ++i) {
            if (y >= ty && y <= ty + 30) {
                if (state == GLUT_DOWN)
                    draggingTaskIndex = i;
                else if (state == GLUT_UP && draggingTaskIndex != -1) {
                    if (i != draggingTaskIndex)
                        swapTasks(i, draggingTaskIndex);
                    draggingTaskIndex = -1;
                    saveTasksToFile();
                }

                if (state == GLUT_DOWN) {
                    if (x >= 610 && x <= 670)
                        tasks[i].done = !tasks[i].done;
                    else if (x >= 680 && x <= 750)
                        tasks.erase(tasks.begin() + i);
                    else if (x >= 760 && x <= 820) {
                        editing = true;
                        editIndex = i;
                        currentInput = tasks[i].text + " ~ " + tasks[i].deadline;
                    }
                    saveTasksToFile();
                }
                break;
            }
            ty -= taskHeight;
        }
    }

    glutPostRedisplay();
}

void keyboard(unsigned char key, int x, int y) {
    if (key == 13) { // Enter
        if (filenameInputMode) {
            if (!currentInput.empty()) {
                fileName = currentInput;
                std::ofstream ofs(fileName, std::ios::app);
                ofs.close();
                loadTasksFromFile();
                currentInput = "";
                filenameInputMode = false;
            }
        } else {
            if (!currentInput.empty()) {
                size_t tildePos = currentInput.find('~');
                std::string taskText = currentInput;
                std::string deadline = "No Deadline";

                if (tildePos != std::string::npos) {
                    taskText = currentInput.substr(0, tildePos);
                    deadline = currentInput.substr(tildePos + 1);
                }

                if (editing && editIndex >= 0 && editIndex < (int)tasks.size()) {
                    tasks[editIndex].text = taskText;
                    tasks[editIndex].deadline = deadline;
                    editing = false;
                    editIndex = -1;
                } else {
                    tasks.push_back({taskText, false, deadline});
                }
                currentInput = "";
                saveTasksToFile();
            }
        }
    } else if (key == 8) {
        if (!currentInput.empty())
            currentInput.pop_back();
    } else if (key == 27) {
        exit(0);
    } else {
        currentInput += key;
    }

    glutPostRedisplay();
}

void specialKeys(int key, int x, int y) {
    if (filenameInputMode) return;

    int maxOffset = -(int)(tasks.size() * taskHeight - (windowHeight - 100));
    if (maxOffset > 0) maxOffset = 0;

    if (key == GLUT_KEY_DOWN)
        scrollOffset -= taskHeight;
    else if (key == GLUT_KEY_UP)
        scrollOffset += taskHeight;

    if (scrollOffset > 0) scrollOffset = 0;
    if (scrollOffset < maxOffset) scrollOffset = maxOffset;

    glutPostRedisplay();
}

// ================= Main ===================
int main(int argc, char** argv) {
    glutInit(&argc, argv);
    glutInitWindowSize(windowWidth, windowHeight); 
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
    glutCreateWindow("To-Do List App with Order and Done/Undone");

    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutMouseFunc(mouse);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(specialKeys);

    glutMainLoop();
    return 0;
}
