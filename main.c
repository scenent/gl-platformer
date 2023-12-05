/* macros */
#define register_just_keys for (int i = 0; i < 349; i++) { current_event.pressed_keys[i] = glfwGetKey(window, i); }
#define clear_frame_event for (int i = 0; i < 349; i++) { current_event.pressed_keys[i] = false; key_info k = { .scancode = 0, .action = 0, .mods = 0 }; current_event.just_keys[i] = k; }

/* standard headers */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <Windows.h>

/* opengl binding headers */
#include <GL/glew.h>
#include <GLFW/glfw3.h>

/* 2-dimensional vector structure implementation */
typedef struct _vec2 {
    float x, y;
} vec2;

/* enumurate direction that working with vec2 */
typedef enum _direction {
    LEFT,
    RIGHT,
    UP,
    DOWN, 
    UNKNOWN
} direction ;

direction get_vec_direction(vec2 v) {
    if (abs(v.x) > abs(v.y)) {
        if (v.x > 0) { return RIGHT; }
        else if (v.x < 0) { return LEFT; }
        else { return UNKNOWN; }
    }
    else {
        if (v.y > 0) { return UP; }
        else if (v.y < 0) { return DOWN; }
        else { return UNKNOWN; }
    }
}

/* polygon structure with local points and global position */
typedef struct _polygon {
    unsigned char color_r, color_g, color_b;
    bool is_static, is_player;
    size_t  plen;
    vec2* points;
    vec2 position;
} polygon;

/* projection structure for SAT algorithm */
typedef struct _projection {
    float min, max;
} projection;

/* result for SAT algorithm */
typedef struct _SATresult {
    vec2 normal;
    bool is_intersect;
} SATresult;

/* frame event structure for game loop */
typedef struct _key_info {
    int scancode, action, mods;
} key_info;

typedef struct _frame_event {
    key_info just_keys[349];
    bool pressed_keys[349];
} frame_event;

/* functions declaration */
vec2 new_vec2(float x, float y);
vec2 add_vec2(vec2 v1, vec2 v2);
vec2 sub_vec2(vec2 v1, vec2 v2);
vec2 mul_vec2(vec2 v1, float f);
vec2 div_vec2(vec2 v1, float f);
float dot_vec2(vec2 v1, vec2 v2);
float cross_vec2(vec2 v1, vec2 v2);
float length_vec2(vec2 v);
vec2 normalize_vec2(vec2 v);
void print_vec2(vec2 v);
void println_vec2(vec2 v);
polygon new_polygon(vec2 position, vec2 points[], size_t plen, bool is_player, bool is_static, unsigned char color_r, unsigned char color_g, unsigned char color_b);
void free_polygon(polygon* polygon);
void print_polygon(polygon p);
vec2* get_global_points(polygon p);
vec2 get_centroid(polygon p);
vec2 perpendicular(vec2* points, size_t plen, int i0, int i1);
projection project(vec2* points, size_t plen, vec2 normal);
float overlap(projection p0, projection p1);
SATresult SAT(polygon body, polygon other);

static vec2 new_vec2(float x, float y) { vec2 v = { .x = x, .y = y }; return v; }
static vec2 add_vec2(vec2 v1, vec2 v2) { vec2 v =  { .x = (v1.x + v2.x), .y = (v1.y + v2.y) }; return v; }
static vec2 sub_vec2(vec2 v1, vec2 v2) { vec2 v = { .x = (v1.x - v2.x), .y = (v1.y - v2.y) }; return v; }
static vec2 mul_vec2(vec2 v1, float f) { vec2 v = { .x = (v1.x * f), .y = (v1.y * f) }; return v; }
static vec2 div_vec2(vec2 v1, float f) { vec2 v = { .x = (v1.x / f), .y = (v1.y / f) }; return v; }
static float dot_vec2(vec2 v1, vec2 v2) { return v1.x * v2.x + v1.y * v2.y; }
static float cross_vec2(vec2 v1, vec2 v2) { return roundf((v1.x * v2.y - v1.y * v2.x) * 100.0f) / 100.0f;}
static float length_vec2(vec2 v) { return sqrtf(powf(v.x, 2.0f) + powf(v.y, 2.0f)); }
static vec2 normalize_vec2(vec2 v) { float len = length_vec2(v); v.x /= len; v.y /= len; return v; }
static void print_vec2(vec2 v) { printf("(%f, %f) ", v.x, v.y); }
static void println_vec2(vec2 v) { printf("(%f, %f)\n", v.x, v.y); }

#pragma region polygon
static polygon new_polygon(vec2 position, vec2 points[], size_t plen, bool is_player, bool is_static, unsigned char color_r, unsigned char color_g, unsigned char color_b) {
    if (plen == 0) {printf("new_polygon() : empty points were given. abort.\n"); exit(1); }
    polygon result;
    result.plen = plen;
    result.position = position;
    result.is_player = is_player;
    result.is_static = is_static;
    result.color_r = color_r;
    result.color_g = color_g;
    result.color_b = color_b;
    result.points = (vec2*)malloc(plen * sizeof(vec2));
    for (size_t i = 0; i < plen; i++) {
        result.points[i] = points[i];
    }
    return result;
}

static void free_polygon(polygon* polygon) {
    free(polygon->points);
    free(polygon);
}

static void print_polygon(polygon p) {
    println_vec2(p.position);
    for (size_t i = 0; i < p.plen; i++) {
        print_vec2(p.points[i]);
    }
    printf("\n");
    return;
}

static void draw_polygon(polygon p, int WINDOW_WIDTH, int WINDOW_HEIGHT, int WINDOW_WIDTH_ORIGIN, int WINDOW_HEIGHT_ORIGIN) {
    glViewport(0.0f, 0.0f, WINDOW_WIDTH, WINDOW_HEIGHT);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, WINDOW_WIDTH_ORIGIN, 0, WINDOW_HEIGHT_ORIGIN, 0, 1);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    // glPushMatrix();
    glBegin(GL_POLYGON);
    glColor4f(p.color_r, p.color_g, p.color_b, 1.0f);
    vec2* global_points = get_global_points(p);
    for (size_t i = 0; i < p.plen; i++) {
        glVertex2f(global_points[i].x, WINDOW_HEIGHT_ORIGIN - global_points[i].y);
    }
    glEnd();
    free(global_points);
    // glPopMatrix();
    return;
}

/* get global points of given polygon */
static vec2* get_global_points(polygon p) {
    if (sizeof(p.points) == 0) { printf("get_global_points() : polygon which has empty points was given. abort.\n"); exit(1); }
    vec2* result = (vec2*)malloc(p.plen * sizeof(vec2));
    for (size_t i = 0; i < p.plen; i++) {
        result[i] = add_vec2(p.points[i], p.position);
    }
    return result;
};

/* get centroid of given polygon */
static vec2 get_centroid(polygon p) {
    vec2 center = new_vec2(0, 0);
    vec2* global_points = get_global_points(p);
    float A = 0.0f;
    float area = 0.0f;
    for (int i = 0; i < p.plen; i++) {
        vec2 v0 = global_points[i];
        vec2 v1 = global_points[(i + 1) != p.plen ? (i + 1) : 0];
        float b = cross_vec2(v0, v1);
        A += b;
        center = add_vec2(center, mul_vec2(add_vec2(v0, v1), b));
    }
    center = mul_vec2(center, 1.0f / (3.0f * A));
    free(global_points);
    return center;
}
#pragma endregion

#pragma region SAT
/* get perpendicular vector of edge of given index */
static vec2 perpendicular(vec2* points, size_t plen, int i0, int i1) {
    if (i1 == plen) { i1 = 0; i0 = plen - 1; }
    vec2 temp = normalize_vec2(sub_vec2(points[i0], points[i1]));
    vec2 result = { .x = -temp.y, .y = temp.x };
    return result;
}

/* project edges with given normal vector */
static projection project(vec2* points, size_t plen, vec2 normal) {
    float min = INFINITY;
    float max = -INFINITY;
    for (int i = 0; i < plen; i++) {
        float projection = dot_vec2(points[i], normal);
        if (projection < min)
            min = projection;
        if (projection > max)
            max = projection;
    }
    projection result = { .min = min, .max = max };
    return result;
}

/* returns depth value if projections are overlapping */
static float overlap(projection p0, projection p1) {
    return !(p0.min <= p1.max && p0.max >= p1.min) ? 0.0f : min(p0.max, p1.max) - max(p0.min, p1.min);
}

/* SAT algorithm implementation */
static SATresult SAT(polygon body, polygon other) {
    SATresult result = { .normal = new_vec2(0, 0), .is_intersect = true};
    float min_overlap = INFINITY;
    vec2* body_points = get_global_points(body);
    vec2* other_points = get_global_points(other);
    vec2* normals = (vec2*)malloc((body.plen + other.plen) * sizeof(vec2));
    for (int i = 0; i < body.plen; i++)
        normals[i] = perpendicular(body_points, body.plen, i, i + 1);
    for (int i = 0; i < other.plen; i++)
        normals[i + body.plen] = perpendicular(other_points, other.plen, i, i + 1);
    for (int i = 0; i < body.plen + other.plen; i++) {
        vec2 n = normals[i];
        projection body_projection = project(body_points, body.plen, n);
        projection other_projection = project(other_points, other.plen, n);
        float o = overlap(body_projection, other_projection);
        if (!o) {
            SATresult err = { .normal = new_vec2(0, 0), .is_intersect = false };
            free(body_points);
            free(other_points);
            free(normals);
            return err;
        }
        else {
            if (o < min_overlap) {
                min_overlap = o;
                result.normal = mul_vec2(n, min_overlap);
            }
        }
    }
    if (dot_vec2(sub_vec2(get_centroid(body), get_centroid(other)), result.normal) < 0.0f)
        result.normal = mul_vec2(result.normal, -1.0f);
    result.is_intersect = true;
    free(body_points);
    free(other_points);
    free(normals);
    return result;
}

#pragma endregion

#pragma region global_variables

int WINDOW_WIDTH_ORIGIN = 800, WINDOW_HEIGHT_ORIGIN = 800;
int WINDOW_WIDTH = 800, WINDOW_HEIGHT = 800;

double current_time, last_time;
frame_event current_event;

vec2 m_player_gravity = { .x = 0.0f, .y = 40.0f };
vec2 m_player_force = { .x = 0.0f, .y = 0.0f };
vec2 m_player_velocity = { .x = 0.0f, .y = 0.0f };
bool m_player_is_on_floor = false;
bool m_player_is_on_ceiling = false;
polygon m_player;

polygon m_wall_left, m_wall_right, m_floor, m_ceiling, m_platform_1, m_platform_2;

#pragma endregion

#pragma region main

static void update(double delta) {
    if (current_event.pressed_keys[GLFW_KEY_A]) {
        m_player_force.x -= 300.0;
    }
    if (current_event.pressed_keys[GLFW_KEY_D]) {
        m_player_force.x += 300.0;
    }
    if (!m_player_is_on_floor) {
        m_player_velocity = add_vec2(m_player_velocity, m_player_gravity);
    }
    else {
        m_player_velocity.y = 0.0f;
    }
    if ((current_event.just_keys[GLFW_KEY_SPACE].action == GLFW_PRESS ||
        current_event.just_keys[GLFW_KEY_W].action == GLFW_PRESS)
        && m_player_is_on_ceiling == false
        ) {
        m_player_velocity.y -= 1000;
    }
    m_player_velocity = add_vec2(m_player_velocity, mul_vec2(m_player_force, delta));
    m_player.position = add_vec2(m_player.position, mul_vec2(m_player_velocity, delta));
    if (m_player_velocity.x != 0.0f || m_player_velocity.y != 0.0f) {
        m_player_velocity = mul_vec2(m_player_velocity, 0.99);
        if (abs(m_player_velocity.x) < 0.01) m_player_velocity.x = 0.0f;
        if (abs(m_player_velocity.y) < 0.01) m_player_velocity.y = 0.0f;
    }
    m_player_force.x = 0.0f;
    m_player_force.y = 0.0f;
}

static void draw() {
    draw_polygon(m_player, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
    draw_polygon(m_wall_left, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
    draw_polygon(m_wall_right, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
    draw_polygon(m_ceiling, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
    draw_polygon(m_floor, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
    draw_polygon(m_platform_1, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
    draw_polygon(m_platform_2, WINDOW_WIDTH, WINDOW_HEIGHT, WINDOW_WIDTH_ORIGIN, WINDOW_HEIGHT_ORIGIN);
}

static void buffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    WINDOW_WIDTH = width;
    WINDOW_HEIGHT = height;
}

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    key_info k = { .scancode = scancode, .action = action, .mods = mods };
    current_event.just_keys[key] = k;
}

int main(const int argc, const char* argv[]) {
    vec2 m_player_rect[4] = { new_vec2(-10, -10), new_vec2(10, -10), new_vec2(10, 10), new_vec2(-10, 10) };
    m_player = new_polygon(new_vec2(400, 400), m_player_rect, 4, true, false, 1, 1, 1);
    vec2 m_wall_left_rect[4] = { new_vec2(0, 0), new_vec2(50, 0), new_vec2(50, 750), new_vec2(0, 750) };
    m_wall_left = new_polygon(new_vec2(0, 0), m_wall_left_rect, 4, false, true, 0, 1, 0);
    vec2 m_wall_right_rect[4] = { new_vec2(750, 0), new_vec2(800, 0), new_vec2(800, 750), new_vec2(750, 750) };
    m_wall_right = new_polygon(new_vec2(0, 0), m_wall_right_rect, 4, false, true, 0, 1, 0);
    vec2 m_floor_rect[4] = { new_vec2(0, 750), new_vec2(800, 750), new_vec2(800, 800), new_vec2(0, 800) };
    m_floor = new_polygon(new_vec2(0, 0), m_floor_rect, 4, false, true, 0, 1, 0);
    vec2 m_ceiling_rect[4] = { new_vec2(0, 0), new_vec2(800, 0), new_vec2(800, 50), new_vec2(0, 50) };
    m_ceiling = new_polygon(new_vec2(0, 0), m_ceiling_rect, 4, false, true, 0, 1, 0);
    vec2 m_platform_1_rect[4] = { new_vec2(50, 600), new_vec2(300, 600), new_vec2(300, 650), new_vec2(50, 650) };
    m_platform_1 = new_polygon(new_vec2(0, 0), m_platform_1_rect, 4, false, true, 0, 1, 1);
    vec2 m_platform_2_rect[4] = { new_vec2(400, 400), new_vec2(750, 400), new_vec2(750, 450), new_vec2(400, 450) };
    m_platform_2 = new_polygon(new_vec2(0, 0), m_platform_2_rect, 4, false, true, 1, 1, 0);
    
    clear_frame_event;

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Platformer", NULL, NULL);
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, buffer_size_callback);
    glfwSetKeyCallback(window, key_callback);
    glfwSwapInterval(1);
    glewInit();

    polygon* object_list[] = { &m_player, &m_wall_left, &m_wall_right, &m_ceiling, &m_floor, &m_platform_1, &m_platform_2 };
    int olen = 7;

    while (!glfwWindowShouldClose(window)) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        current_time = glfwGetTime();
        while (glfwGetTime() <= last_time + 1.0 / 60.0) current_time = glfwGetTime();
        glfwPollEvents();
        register_just_keys;
        double delta_time = current_time - last_time;
        update(delta_time);
        // apply SAT algorithm
        m_player_is_on_floor = false;
        m_player_is_on_ceiling = false;
        for (int i = 0; i < olen; i++) {
            for (int j = i + 1; j < olen; j++) {
                SATresult result = SAT(*object_list[i], *object_list[j]);
                if (result.is_intersect) {
                    if (!object_list[i]->is_static)
                        object_list[i]->position = add_vec2(object_list[i]->position, result.normal);
                    if (!object_list[j]->is_static)
                        object_list[j]->position = sub_vec2(object_list[j]->position, result.normal);
                    if (object_list[i]->is_player || object_list[j]->is_player) {
                        if (get_vec_direction(result.normal) == DOWN) {
                            m_player_is_on_floor = true;
                        }
                        else if (get_vec_direction(result.normal) == UP) {
                            m_player_is_on_ceiling = true;
                        }
                    }
                }
            }
        }
        draw();
        glfwSwapBuffers(window);
        clear_frame_event;
        last_time = current_time;
    }
    glfwTerminate();
    return 0;
}

#pragma endregion
