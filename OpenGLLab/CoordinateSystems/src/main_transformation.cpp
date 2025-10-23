#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/detail/setup.hpp>   // ← 여기서 GLM_VERSION_* 매크로 제공

int main() {
    std::cout << "GLM version: "
        << GLM_VERSION_MAJOR << "."
        << GLM_VERSION_MINOR << "."
        << GLM_VERSION_PATCH << "\n";

    glm::vec3 p(1.0f, 2.0f, 3.0f);
    glm::mat4 M(1.0f);
    M = glm::translate(M, glm::vec3(10.0f, 0.0f, 0.0f));
    M = glm::rotate(M, glm::radians(90.0f), glm::vec3(0, 0, 1));
    M = glm::scale(M, glm::vec3(2.0f));

    glm::vec4 tp = M * glm::vec4(p, 1.0f);
    std::cout << "p' = (" << tp.x << ", " << tp.y << ", " << tp.z << ", " << tp.w << ")\n";

    const float* m = glm::value_ptr(M);
    std::cout << "M (column-major):\n";
    for (int r = 0; r < 4; ++r) { for (int c = 0; c < 4; ++c) std::cout << m[c * 4 + r] << (c < 3 ? '\t' : '\n'); }
    return 0;
}