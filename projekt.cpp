#include <iostream>
#include <SFML/Graphics.hpp>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <string>
#include <stdexcept>
#include <sstream>
#include <set>
#include <nlohmann/json.hpp>
#include <fstream>
using json = nlohmann::json;

void saveScoreToFile(const std::string &filename, int score) {
    json data;
    std::ifstream fileIn(filename);
    if (fileIn.is_open()) {
        fileIn >> data;
        fileIn.close();
    }

    data["highscore"] = std::max(data.value("highscore", 0), score);

    std::ofstream fileOut(filename);
    if (fileOut.is_open()) {
        fileOut << data.dump(4); // Zapis danych w formacie czytelnym dla człowieka
        fileOut.close();
    }
}

int loadHighScoreFromFile(const std::string &filename) {
    std::ifstream file(filename);
    if (file.is_open()) {
        json data;
        file >> data;
        file.close();
        return data.value("highscore", 0);
    }
    return 0;
}

// Klasa UFO
class Ufo {
private:
    sf::Vector2f position;
    float speed = 200.f;
    sf::Texture tekstura;
    sf::Sprite pSprite;

public:
    Ufo(float x_in, float y_in) {
        position.x = x_in;
        position.y = y_in;
        if (!tekstura.loadFromFile("ufo.png")) {
            throw std::runtime_error("Nie można załadować pliku tekstury UFO");
        }
        pSprite.setTexture(tekstura);
        pSprite.setPosition(position);
    }

    void update(float deltaTime, const sf::FloatRect &bounds) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
            position.x -= speed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
            position.x += speed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
            position.y -= speed * deltaTime;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
            position.y += speed * deltaTime;
        }

        if (position.x < bounds.left) {
            position.x = bounds.left;
        }
        if (position.x + pSprite.getGlobalBounds().width > bounds.left + bounds.width) {
            position.x = bounds.left + bounds.width - pSprite.getGlobalBounds().width;
        }
        if (position.y < bounds.top) {
            position.y = bounds.top;
        }
        if (position.y + pSprite.getGlobalBounds().height > bounds.top + bounds.height) {
            position.y = bounds.top + bounds.height - pSprite.getGlobalBounds().height;
        }

        pSprite.setPosition(position);
    }

    void draw(sf::RenderWindow &window) {
        window.draw(pSprite);
    }

    sf::FloatRect getBounds() const {
        return pSprite.getGlobalBounds();
    }

    sf::Vector2f getPosition() const {
        return position;
    }
};
class Obstacle {
private:
    sf::Sprite sprite;          // Używamy sf::Sprite do wyświetlania tekstury
    static sf::Texture texture; // Statyczna tekstura, wspólna dla wszystkich przeszkód
    float speed;                // Prędkość przeszkody

public:
    // Konstruktor
    Obstacle(float x, float y, float obstacleSpeed = 150.f) : speed(obstacleSpeed) {
        // Ustawienie tekstury tylko raz (dla pierwszej przeszkody)
        if (texture.getSize().x == 0 && texture.getSize().y == 0) {
            if (!texture.loadFromFile("planeta.png")) {
                std::cerr << "Nie udało się załadować tekstury!" << std::endl;
            }
        }

        sprite.setTexture(texture);  // Przypisanie tekstury do sprite'a
        sprite.setPosition(x, y);    // Ustawienie pozycji
    }

    // Aktualizacja pozycji przeszkody
    void update(float deltaTime, const sf::FloatRect &bounds) {
        sf::Vector2f position = sprite.getPosition();
        position.x -= speed * deltaTime;

        // Jeśli przeszkoda wyjeżdża poza lewy brzeg bounds, resetujemy ją w obrębie bounds
        if (position.x + sprite.getGlobalBounds().width < bounds.left) {
            position.x = bounds.left + bounds.width;
            position.y = bounds.top + static_cast<float>(rand() % static_cast<int>(bounds.height - sprite.getGlobalBounds().height));
        }

        sprite.setPosition(position);
    }

    // Rysowanie przeszkody
    void draw(sf::RenderWindow &window) {
        window.draw(sprite);
    }

    // Pobranie granic przeszkody
    sf::FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }
};
class Reward {
private:
    sf::Sprite sprite;
    static sf::Texture texture;
    float speed;

public:
    Reward(float x, float y, float rewardSpeed = 100.f) : speed(rewardSpeed) {
        if (texture.getSize().x == 0 && texture.getSize().y == 0) {
            if (!texture.loadFromFile("kometa.png")) {
                std::cerr << "Nie udało się załadować tekstury nagrody!" << std::endl;
            }
        }

        sprite.setTexture(texture);
        sprite.setPosition(x, y);
    }

    void update(float deltaTime, const sf::FloatRect &bounds) {
        sf::Vector2f position = sprite.getPosition();
        position.x -= speed * deltaTime;

        if (position.x + sprite.getGlobalBounds().width < bounds.left) {
            position.x = bounds.left + bounds.width;
            position.y = bounds.top + static_cast<float>(rand() % static_cast<int>(bounds.height - sprite.getGlobalBounds().height));
        }

        sprite.setPosition(position);
    }

    void draw(sf::RenderWindow &window) {
        window.draw(sprite);
    }

    sf::FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }
};

// Inicjalizowanie statycznej tekstury na poziomie klasy
sf::Texture Reward::texture;

struct Level {
    sf::Color backgroundColor;
    float obstacleSpeed;
    int numObstacles;

    Level(sf::Color bgColor, float speed, int obstacles)
        : backgroundColor(bgColor), obstacleSpeed(speed), numObstacles(obstacles) {}
};

// Inicjalizowanie statycznej tekstury na poziomie klasy
sf::Texture Obstacle::texture;

class Interfejs {
private:
    sf::RectangleShape rectangle;
    sf::Texture backgroundTexture;
    sf::Sprite backgroundSprite;
    sf::Font font;
    sf::Text rightTopText;
    sf::Text scoreText;
    sf::Text helpText;
    sf::Text pauseText;
    bool showHelp = false;  // Flaga dla ekranu pomocy (Tab)
    bool showPause = false; // Flaga dla ekranu pauzy (Escape)
    bool exitRequested = false; // Flaga wskazująca, czy użytkownik chce zamknąć okno
    sf::Vector2f size;

    void init() {
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Nie można załadować pliku czcionki");
        }

        rightTopText.setFont(font);
        rightTopText.setCharacterSize(20);
        rightTopText.setFillColor(sf::Color::White);
        rightTopText.setPosition(size.x - 200, 5);

        scoreText.setFont(font);
        scoreText.setCharacterSize(20);
        scoreText.setFillColor(sf::Color::Yellow);
        scoreText.setPosition(10, 5);

        rectangle.setSize(sf::Vector2f(size.x, size.y - 100));
        rectangle.setPosition((size.x - rectangle.getSize().x), (size.y - rectangle.getSize().y) / 2);
        rectangle.setFillColor(sf::Color::Transparent);
        rectangle.setOutlineThickness(1.f);
        rectangle.setOutlineColor(sf::Color::White);

        if (!backgroundTexture.loadFromFile("space.jpg")) {
            throw std::runtime_error("Nie można załadować pliku tekstury tła");
        }

        backgroundSprite.setTexture(backgroundTexture);
        sf::Vector2f scale(
            rectangle.getSize().x / backgroundTexture.getSize().x,
            rectangle.getSize().y / backgroundTexture.getSize().y
        );
        backgroundSprite.setScale(scale);
        backgroundSprite.setPosition(rectangle.getPosition());

        helpText.setFont(font);
        helpText.setString("Pomoc: Celem gry jest omijanie za pomoca strzalek innych statkow kosmicznych\n\nAby wznowic rozgrywke prosze nacisnac Tab\n\nAby zakonczyc gre prosze nacisnac ECS");
        helpText.setCharacterSize(30);
        helpText.setFillColor(sf::Color::White);

        pauseText.setFont(font);
        pauseText.setString("\n\n\n\n\n\n\n\n\nWcisnij ESC aby napewno zakonczyc rozgrywke\n\n Aby kontynuowac rozgrywke wcisnij Shift");
        pauseText.setCharacterSize(30);
        pauseText.setFillColor(sf::Color::Red);
    }

    void centerText(sf::Text &text) {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition((size.x - textBounds.width) / 2, (size.y - textBounds.height) / 2);
    }

public:
    Interfejs(const sf::Vector2f &windowSize) : size(windowSize) {
        init();
        centerText(helpText);
        centerText(pauseText);
    }

    void setText(const std::string &right, int score) {
        rightTopText.setString(right);
        scoreText.setString("Punkty: " + std::to_string(score));
    }

    void updateTexts(const sf::Vector2f &position, int score) {
        std::ostringstream ss;
        ss << "Pozycja: (" << static_cast<int>(position.x) << ", " << static_cast<int>(position.y) << ")";
        rightTopText.setString(ss.str());
        scoreText.setString("Punkty: " + std::to_string(score));
    }

    void toggleHelp() {
        showHelp = !showHelp;
    }

    void togglePause() {
        showPause = !showPause;
    }

    void resumeGame() {
        showPause = false;
    }

    void requestExit() {
        exitRequested = true;
    }

    bool isHelpVisible() const {
        return showHelp;
    }

    bool isPauseVisible() const {
        return showPause;
    }

    bool isExitRequested() const {
        return exitRequested;
    }

    sf::FloatRect getCentralBounds() const {
        return rectangle.getGlobalBounds();
    }

    void draw(sf::RenderWindow &window) {
        window.draw(backgroundSprite);
        window.draw(rectangle);
        window.draw(rightTopText);
        window.draw(scoreText);

        if (showHelp) {
            window.draw(helpText);
        }

        if (showPause) {
            window.draw(pauseText);
        }
    }
};

int main() {
    try {
        srand(static_cast<unsigned int>(time(nullptr)));

        sf::Vector2f windowSize(1200, 750);
        sf::RenderWindow window(sf::VideoMode(static_cast<unsigned int>(windowSize.x), static_cast<unsigned int>(windowSize.y)), "Space Game");

        Interfejs interfejs(windowSize);
        interfejs.setText("Pozycja: (0, 0)", 10);

        sf::FloatRect centralBounds = interfejs.getCentralBounds();
        Ufo ufo(centralBounds.left + centralBounds.width / 2 - 25.f, centralBounds.top + centralBounds.height / 2 - 25.f);

        // Definiowanie poziomów
        struct Level {
            sf::Color backgroundColor;
            float obstacleSpeed;
            int numObstacles;
        };

        std::vector<Level> levels = {
            {sf::Color::Black, 100.f, 5},   // Poziom 1
            {sf::Color::Cyan, 150.f, 8},   // Poziom 2
            {sf::Color::Magenta, 200.f, 10}, // Poziom 3
        };
        int currentLevelIndex = 0;
        Level currentLevel = levels[currentLevelIndex];

        // Generowanie przeszkód
        std::vector<Obstacle> obstacles;
        auto initializeObstacles = [&]() {
            obstacles.clear();
            for (int i = 0; i < currentLevel.numObstacles; ++i) {
                float x = centralBounds.left + static_cast<float>(rand() % static_cast<int>(centralBounds.width));
                float y = centralBounds.top + static_cast<float>(rand() % static_cast<int>(centralBounds.height));
                obstacles.emplace_back(x, y, currentLevel.obstacleSpeed);
            }
        };
        initializeObstacles();

        // Generowanie nagród
        std::vector<Reward> rewards;
        auto initializeRewards = [&]() {
            rewards.clear();
            for (int i = 0; i < 3; ++i) { // Na przykład, 3 nagrody na poziom
                float x = centralBounds.left + static_cast<float>(rand() % static_cast<int>(centralBounds.width));
                float y = centralBounds.top + static_cast<float>(rand() % static_cast<int>(centralBounds.height));
                rewards.emplace_back(x, y, currentLevel.obstacleSpeed / 2); // Nagrody poruszają się wolniej
            }
        };
        initializeRewards();

        int score = 10;
        sf::Clock clock;

        while (window.isOpen()) {
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();

                if (event.type == sf::Event::KeyPressed) {
                    if (event.key.code == sf::Keyboard::F1) {
                        interfejs.toggleHelp();
                    } else if (event.key.code == sf::Keyboard::Escape) {
                        if (interfejs.isPauseVisible()) {
                            interfejs.requestExit();
                        } else {
                            interfejs.togglePause();
                        }
                    } else if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
                        if (interfejs.isPauseVisible()) {
                            interfejs.resumeGame();
                        }
                    } else if (event.key.code == sf::Keyboard::Return) {
                        // Przełączanie poziomów
                        currentLevelIndex = (currentLevelIndex + 1) % levels.size();
                        currentLevel = levels[currentLevelIndex];
                        initializeObstacles(); // Odświeżenie przeszkód
                        initializeRewards();   // Odświeżenie nagród
                    }
                }
            }

            if (interfejs.isExitRequested()) {
                window.close();
            }

            float deltaTime = clock.restart().asSeconds();

            if (!interfejs.isHelpVisible() && !interfejs.isPauseVisible()) {
                ufo.update(deltaTime, centralBounds);

                for (auto &obstacle : obstacles) {
                    obstacle.update(deltaTime, centralBounds);

                    if (ufo.getBounds().intersects(obstacle.getBounds())) {
                        static sf::Clock collisionClock;
                        if (collisionClock.getElapsedTime().asMilliseconds() > 500) {
                            score -= 1;
                            collisionClock.restart();
                        }
                    }
                }

                for (auto &reward : rewards) {
                    reward.update(deltaTime, centralBounds);

                    if (ufo.getBounds().intersects(reward.getBounds())) {
                        static sf::Clock rewardCollisionClock;
                        if (rewardCollisionClock.getElapsedTime().asMilliseconds() > 500) {
                            score += 2; // Dodaj punkty za nagrodę
                            rewardCollisionClock.restart();

                            // Resetuj pozycję nagrody
                            float x = centralBounds.left + centralBounds.width;
                            float y = centralBounds.top + static_cast<float>(rand() % static_cast<int>(centralBounds.height - reward.getBounds().height));
                            reward = Reward(x, y, currentLevel.obstacleSpeed / 2);
                        }
                    }
                }

                interfejs.updateTexts(ufo.getPosition(), score);
            }

            // Rysowanie
            window.clear(currentLevel.backgroundColor);

            interfejs.draw(window);

            if (!interfejs.isHelpVisible() && !interfejs.isPauseVisible()) {
                for (auto &obstacle : obstacles) {
                    obstacle.draw(window);
                }

                for (auto &reward : rewards) {
                    reward.draw(window);
                }

                ufo.draw(window);
            }

            window.display();
        }
    } catch (const std::exception &e) {
        std::cerr << "Wyjątek: " << e.what() << std::endl;
    }
    return 0;
}
