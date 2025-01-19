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

// Struktura przechowująca dane o stanie gry
// Zawiera pozycję gracza, wynik i datę zapisu
struct GameData {
    sf::Vector2f position;
    int score;
    std::string date;
};

// Funkcja zapisująca stan gry do pliku JSON
// Parametry:
// - filename: nazwa pliku do zapisu
// - gameDataList: lista danych gry do zapisania
void saveScoreToJson(const std::string& filename, const std::vector<GameData>& gameDataList) {
    nlohmann::json jsonData;
    std::ifstream inputFile(filename);
    if (inputFile.is_open()) {
        try {
            inputFile >> jsonData;
        } catch (const std::exception& e) {
            std::cerr << "Błąd podczas wczytywania istniejących danych: " << e.what() << std::endl;
        }
        inputFile.close();
    }

    // Konwersja danych gry do formatu JSON
    for (const auto& gameData : gameDataList) {
        nlohmann::json entry;
        entry["position"] = {gameData.position.x, gameData.position.y};
        entry["score"] = gameData.score;
        entry["date"] = gameData.date;
        jsonData["games"].push_back(entry);
    }

    // Zapis do pliku
    std::ofstream outputFile(filename, std::ios::trunc);
    if (!outputFile.is_open()) {
        std::cerr << "Nie można otworzyć pliku do zapisu!" << std::endl;
        return;
    }
    outputFile << jsonData.dump(4);
    outputFile.close();
}

// Funkcja wczytująca stan gry z pliku JSON
// Parametry:
// - filename: nazwa pliku do odczytu
// - gameDataList: lista na wczytane dane
// - ufoPosition: pozycja gracza do zaktualizowania
// - score: wynik do zaktualizowania
void loadScoreFromJson(const std::string& filename, std::vector<GameData>& gameDataList, sf::Vector2f& ufoPosition, int& score) {
    std::ifstream inputFile(filename);
    if (!inputFile.is_open()) {
        std::cerr << "Nie udało się otworzyć pliku do wczytania stanu gry!" << std::endl;
        return;
    }

    // Wczytanie i parsowanie JSON
    nlohmann::json jsonData;
    try {
        inputFile >> jsonData;
    } catch (const std::exception& e) {
        std::cerr << "Błąd podczas wczytywania danych JSON: " << e.what() << std::endl;
        return;
    }
    inputFile.close();

    gameDataList.clear();

    // Konwersja danych JSON na obiekty GameData
    if (jsonData.contains("games")) {
        for (const auto& entry : jsonData["games"]) {
            GameData gameData;
            gameData.position = sf::Vector2f(entry["position"][0], entry["position"][1]);
            gameData.score = entry["score"];
            gameData.date = entry["date"];
            gameDataList.push_back(gameData);
        }

        // Aktualizacja stanu gry ostatnim zapisem
        if (!gameDataList.empty()) {
            const auto& lastGameData = gameDataList.back();
            ufoPosition = lastGameData.position;
            score = lastGameData.score;
        }
    }
}

class Interfejs {
private:
    sf::Text gameOverText;          // Tekst "Game Over"
    bool isGameOver = false;        // Flaga końca gry
    sf::RectangleShape rectangle;   // Prostokąt obszaru gry
    sf::Texture backgroundTexture;  // Tekstura tła
    sf::Sprite backgroundSprite;    // Sprite tła
    sf::Font font;                  // Czcionka
    sf::Text rightTopText;          // Tekst w prawym górnym rogu
    sf::Text bottomText;            // Tekst na dole ekranu
    sf::Text scoreText;             // Tekst wyniku
    sf::Text helpText;              // Tekst pomocy
    sf::Text pauseText;             // Tekst pauzy
    bool showHelp = false;          // Flaga wyświetlania pomocy
    bool showPause = false;         // Flaga pauzy
    bool exitRequested = false;     // Flaga żądania wyjścia
    sf::Vector2f size;              // Rozmiar okna

    // Inicjalizacja komponentów interfejsu
    void init() {
        // Ładowanie czcionki
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Nie można załadować pliku czcionki");
        }

        // Konfiguracja tekstu w prawym górnym rogu
        rightTopText.setFont(font);
        rightTopText.setCharacterSize(20);
        rightTopText.setFillColor(sf::Color::Yellow);
        rightTopText.setPosition(size.x - 200, 5);

        // Konfiguracja tekstu na dole
        bottomText.setFont(font);
        bottomText.setString("---------------Pomoc F1-------------------------------------------------------------------------------------------------------------------------------------------Menu M---------------");
        bottomText.setCharacterSize(20);
        bottomText.setFillColor(sf::Color::Yellow);
        bottomText.setPosition(10, size.y - bottomText.getLocalBounds().height - 20);

        // Konfiguracja tekstu wyniku
        scoreText.setFont(font);
        scoreText.setCharacterSize(20);
        scoreText.setFillColor(sf::Color::Yellow);
        scoreText.setPosition(10, 5);

        // Konfiguracja prostokąta obszaru gry
        rectangle.setSize(sf::Vector2f(size.x, size.y - 100));
        rectangle.setPosition((size.x - rectangle.getSize().x), (size.y - rectangle.getSize().y) / 2);
        rectangle.setFillColor(sf::Color::Transparent);
        rectangle.setOutlineThickness(1.f);
        rectangle.setOutlineColor(sf::Color::White);

        // Ładowanie i konfiguracja tła
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

        // Konfiguracja tekstu pomocy
        helpText.setFont(font);
        helpText.setString("Menu kliknij M\n\nAby wznowic rozgrywke prosze nacisnac F1\n\nAby zakonczyc gre prosze nacisnac ECS");
        helpText.setCharacterSize(30);
        helpText.setFillColor(sf::Color::White);

        // Konfiguracja tekstu pauzy
        pauseText.setFont(font);
        pauseText.setString("\n\n\n\n\n\n\n\n\nWcisnij ESC aby napewno zakonczyc rozgrywke\n\n Aby kontynuowac rozgrywke wcisnij Shift");
        pauseText.setCharacterSize(30);
        pauseText.setFillColor(sf::Color::Red);
        
        centerText(helpText);
        centerText(pauseText);
    }

    // Wycentrowanie tekstu na ekranie
    void centerText(sf::Text &text) {
        sf::FloatRect textBounds = text.getLocalBounds();
        text.setPosition((size.x - textBounds.width) / 2, (size.y - textBounds.height) / 2);
    }

public:
    // Konstruktor
    Interfejs(const sf::Vector2f &windowSize) : size(windowSize) {
        init();
    }

    // Ustawienie tekstów w interfejsie
    void setText(const std::string &right, int score) {
        rightTopText.setString(right);
        scoreText.setString("Punkty: " + std::to_string(score));
    }

    // Aktualizacja tekstów w interfejsie
    void updateTexts(const sf::Vector2f &position, int score) {
        std::ostringstream ss;
        ss << "Pozycja: (" << static_cast<int>(position.x) << ", " << static_cast<int>(position.y) << ")";
        rightTopText.setString(ss.str());
        scoreText.setString("Punkty: " + std::to_string(score));
    }

    // Pokazanie tekstu końca gry
    void showGameOver() {
        gameOverText.setString("Game Over");
        centerText(gameOverText);
    }

    // Metody zarządzające stanem interfejsu
    void toggleHelp() { showHelp = !showHelp; }
    void togglePause() { showPause = !showPause; }
    void resumeGame() { showPause = false; }
    void requestExit() { exitRequested = true; }
    bool isHelpVisible() const { return showHelp; }
    bool isPauseVisible() const { return showPause; }
    bool isExitRequested() const { return exitRequested; }
    sf::FloatRect getCentralBounds() const { return rectangle.getGlobalBounds(); }

    // Rysowanie interfejsu
    void draw(sf::RenderWindow &window) {
        window.draw(backgroundSprite);
        window.draw(rectangle);
        window.draw(bottomText);
        window.draw(rightTopText);
        window.draw(scoreText);
        
        if (isGameOver) {
            window.draw(gameOverText);
        }
        if (showHelp) {
            window.draw(helpText);
        }
        if (showPause) {
            window.draw(pauseText);
        }
    }
};


// Klasa UFO
class Ufo {
private:
    sf::Vector2f position;
    float speed = 200.f;
    sf::Texture texture;
    sf::Sprite sprite;

public:
    // Konstruktor
    Ufo(float x_in, float y_in) {
        position.x = x_in;
        position.y = y_in;
        if (!texture.loadFromFile("ufo.png")) {
            throw std::runtime_error("Nie można załadować pliku tekstury UFO");
        }
        sprite.setTexture(texture);
        sprite.setPosition(position);
    }

    // Metoda aktualizująca pozycję UFO
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

        // Utrzymanie w granicach ekranu
        if (position.x < bounds.left) {
            position.x = bounds.left;
        }
        if (position.x + sprite.getGlobalBounds().width > bounds.left + bounds.width) {
            position.x = bounds.left + bounds.width - sprite.getGlobalBounds().width;
        }
        if (position.y < bounds.top) {
            position.y = bounds.top;
        }
        if (position.y + sprite.getGlobalBounds().height > bounds.top + bounds.height) {
            position.y = bounds.top + bounds.height - sprite.getGlobalBounds().height;
        }

        sprite.setPosition(position);
    }

    // Metoda rysująca UFO
    void draw(sf::RenderWindow &window) {
        window.draw(sprite);
    }

    // Pobranie prostokąta granicznego UFO
    sf::FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }

    // Pobranie pozycji UFO
    sf::Vector2f getPosition() const {
        return position;
    }

    // Ustawienie pozycji UFO (dla ładowania z pliku)
    void setPosition(const sf::Vector2f &newPosition) {
        position = newPosition;
        sprite.setPosition(position);
    }
};

// Klasa reprezentująca przeszkody w grze
class Obstacle {
private:
    sf::Sprite sprite;          // Obiekt sprite używany do wyświetlania przeszkody na ekranie
    static sf::Texture texture; // Współdzielona tekstura używana przez wszystkie przeszkody dla optymalizacji pamięci
    float speed;                // Prędkość poruszania się przeszkody (w pikselach na sekundę)

public:
    // Konstruktor inicjalizujący przeszkodę
    // Parametry:
    // - x: początkowa pozycja x przeszkody
    // - y: początkowa pozycja y przeszkody
    // - obstacleSpeed: prędkość poruszania się przeszkody (domyślnie 150.f pikseli/s)
    Obstacle(float x, float y, float obstacleSpeed = 150.f) : speed(obstacleSpeed) {
        // Ładowanie tekstury tylko przy pierwszym utworzeniu obiektu
        // Sprawdzanie czy tekstura już istnieje poprzez sprawdzenie jej wymiarów
        if (texture.getSize().x == 0 && texture.getSize().y == 0) {
            if (!texture.loadFromFile("planeta.png")) {
                std::cerr << "Nie udało się załadować tekstury!" << std::endl;
            }
        }
        
        sprite.setTexture(texture);  // Przypisanie tekstury do sprite'a
        sprite.setPosition(x, y);    // Ustawienie początkowej pozycji przeszkody
    }

    // Aktualizacja pozycji przeszkody w każdej klatce gry
    // Parametry:
    // - deltaTime: czas od ostatniej aktualizacji (w sekundach)
    // - bounds: granice obszaru gry
    void update(float deltaTime, const sf::FloatRect &bounds) {
        sf::Vector2f position = sprite.getPosition();
        position.x -= speed * deltaTime;  // Przesunięcie przeszkody w lewo

        // Reset pozycji przeszkody gdy wyjdzie poza lewą krawędź ekranu
        // Przeszkoda pojawia się ponownie po prawej stronie na losowej wysokości
        if (position.x + sprite.getGlobalBounds().width < bounds.left) {
            position.x = bounds.left + bounds.width;
            position.y = bounds.top + static_cast<float>(rand() % static_cast<int>(bounds.height - sprite.getGlobalBounds().height));
        }

        sprite.setPosition(position);
    }

    // Rysowanie przeszkody w oknie gry
    // Parametry:
    // - window: referencja do okna gry, w którym ma być narysowana przeszkoda
    void draw(sf::RenderWindow &window) const {
        window.draw(sprite);
    }

    // Zwraca prostokąt ograniczający przeszkodę
    // Używany do sprawdzania kolizji z innymi obiektami
    // Zwraca: sf::FloatRect reprezentujący granice przeszkody
    sf::FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }
};

// Klasa reprezentująca nagrody w grze
class Reward {
private:
    sf::Sprite sprite;          // Obiekt sprite używany do wyświetlania nagrody
    static sf::Texture texture; // Współdzielona tekstura używana przez wszystkie nagrody
    float speed;                // Prędkość poruszania się nagrody (w pikselach na sekundę)

public:
    // Konstruktor inicjalizujący nagrodę
    // Parametry:
    // - x: początkowa pozycja x nagrody
    // - y: początkowa pozycja y nagrody
    // - rewardSpeed: prędkość poruszania się nagrody (domyślnie 100.f pikseli/s)
    Reward(float x, float y, float rewardSpeed = 100.f) : speed(rewardSpeed) {
        // Ładowanie tekstury tylko przy pierwszym utworzeniu obiektu
        if (texture.getSize().x == 0 && texture.getSize().y == 0) {
            if (!texture.loadFromFile("kometa.png")) {
                std::cerr << "Nie udało się załadować tekstury nagrody!" << std::endl;
            }
        }

        sprite.setTexture(texture);  // Przypisanie tekstury do sprite'a
        sprite.setPosition(x, y);    // Ustawienie początkowej pozycji nagrody
    }

    // Aktualizacja pozycji nagrody w każdej klatce gry
    // Parametry:
    // - deltaTime: czas od ostatniej aktualizacji (w sekundach)
    // - bounds: granice obszaru gry
    void update(float deltaTime, const sf::FloatRect &bounds) {
        sf::Vector2f position = sprite.getPosition();
        position.x -= speed * deltaTime;  // Przesunięcie nagrody w lewo

        // Reset pozycji nagrody gdy wyjdzie poza lewą krawędź ekranu
        // Nagroda pojawia się ponownie po prawej stronie na losowej wysokości
        if (position.x + sprite.getGlobalBounds().width < bounds.left) {
            position.x = bounds.left + bounds.width;
            position.y = bounds.top + static_cast<float>(rand() % static_cast<int>(bounds.height - sprite.getGlobalBounds().height));
        }

        sprite.setPosition(position);
    }

    // Rysowanie nagrody w oknie gry
    // Parametry:
    // - window: referencja do okna gry, w którym ma być narysowana nagroda
    void draw(sf::RenderWindow &window) const {
        window.draw(sprite);
    }

    // Zwraca prostokąt ograniczający nagrodę
    // Używany do sprawdzania kolizji z innymi obiektami
    // Zwraca: sf::FloatRect reprezentujący granice nagrody
    sf::FloatRect getBounds() const {
        return sprite.getGlobalBounds();
    }
};
// Inicjalizowanie statycznej tekstury na poziomie klasy
sf::Texture Reward::texture;

// Struktura przechowująca konfigurację poziomu gry
struct Level {
    sf::Color backgroundColor;  // Kolor tła dla danego poziomu
    float obstacleSpeed;        // Prędkość przeszkód na tym poziomie
    int numObstacles;          // Liczba przeszkód na poziomie

    // Konstruktor poziomu inicjalizujący wszystkie parametry
    // Parametry:
    // - bgColor: kolor tła poziomu
    // - speed: prędkość przeszkód
    // - obstacles: liczba przeszkód
    Level(sf::Color bgColor, float speed, int obstacles)
        : backgroundColor(bgColor), obstacleSpeed(speed), numObstacles(obstacles) {}
};

// Inicjalizacja statycznej tekstury dla klasy Obstacle
sf::Texture Obstacle::texture;

// Klasa zarządzająca różnymi ekranami gry (menu, gra, koniec gry)
class ScreenManager {
public:
    // Typ wyliczeniowy określający możliwe ekrany w grze
    enum class ScreenType {
        Game,   // Ekran głównej rozgrywki
        Ende,   // Ekran końca gry
        Los     // Ekran menu/opcji
    };

private:
    ScreenType currentScreen;   // Aktualnie wyświetlany ekran
    sf::Font font;             // Czcionka używana do wyświetlania tekstów
    sf::Text endeText;         // Tekst wyświetlany na ekranie końca gry
    sf::Text losText;          // Tekst wyświetlany w menu

    // Inicjalizacja ekranu końca gry
    // Konfiguruje tekst, czcionkę i pozycję dla ekranu "Ende"
    // Parametry:
    // - windowSize: rozmiar okna gry do wycentrowania tekstu
    void initEndeScreen(const sf::Vector2f &windowSize) {
        // Ładowanie czcionki
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Nie można załadować pliku czcionki");
        }

        // Konfiguracja tekstu końca gry
        endeText.setFont(font);
        endeText.setString("Ende\n\nESC - koniec rozgrywki\n\n G - Kontynucja");
        endeText.setCharacterSize(50);
        endeText.setFillColor(sf::Color::Red);

        // Centrowanie tekstu na ekranie
        sf::FloatRect textBounds = endeText.getLocalBounds();
        endeText.setPosition(
            (windowSize.x - textBounds.width) / 2,
            (windowSize.y - textBounds.height) / 2
        );
    }

    // Inicjalizacja ekranu menu
    // Konfiguruje tekst, czcionkę i pozycję dla ekranu "Los"
    // Parametry:
    // - windowSize: rozmiar okna gry do wycentrowania tekstu
    void initLosScreen(const sf::Vector2f &windowSize) {
        // Ładowanie czcionki
        if (!font.loadFromFile("arial.ttf")) {
            throw std::runtime_error("Nie można załadować pliku czcionki");
        }

        // Konfiguracja tekstu menu
        losText.setFont(font);
        losText.setString("Menu\n\nGra polega na pomijaniu innych statkow kosmiczych \n\nprzy jednoczesnym zbieraniu monet\n\nReturn - Zmiana poziomow\n\nF1 - Pomoc\n\nESC - Koniec gry\n\nS - Zapis gry\n\nF - Przywrocenie ostatniego zapisu");
        losText.setCharacterSize(30);
        losText.setFillColor(sf::Color::Blue);

        // Centrowanie tekstu na ekranie
        sf::FloatRect textBounds = losText.getLocalBounds();
        losText.setPosition(
            (windowSize.x - textBounds.width) / 2,
            (windowSize.y - textBounds.height) / 2
        );
    }

public:
    // Konstruktor inicjalizujący menedżera ekranów
    // Parametry:
    // - windowSize: rozmiar okna gry
    ScreenManager(const sf::Vector2f &windowSize)
        : currentScreen(ScreenType::Game) {
        initEndeScreen(windowSize);
        initLosScreen(windowSize);
    }

    // Przełączanie między różnymi ekranami
    // Parametry:
    // - screen: typ ekranu, na który należy przełączyć
    void switchTo(ScreenType screen) {
        currentScreen = screen;
    }

    // Pobranie aktualnie wyświetlanego ekranu
    // Zwraca: aktualny typ ekranu
    ScreenType getCurrentScreen() const {
        return currentScreen;
    }

    // Rysowanie odpowiedniego ekranu w zależności od aktualnego stanu
    // Parametry:
    // - window: okno gry do rysowania
    // - interfejs: interfejs gry do wyświetlenia
    // - ufo: obiekt gracza do wyświetlenia
    // - obstacles: wektor przeszkód do wyświetlenia
    // - rewards: wektor nagród do wyświetlenia
    void draw(sf::RenderWindow &window, Interfejs &interfejs, Ufo &ufo, const std::vector<Obstacle> &obstacles, const std::vector<Reward> &rewards) {
        if (currentScreen == ScreenType::Game) {
            // Rysowanie ekranu gry ze wszystkimi elementami
            interfejs.draw(window);
            for (const auto &obstacle : obstacles) {
                obstacle.draw(window);
            }
            for (const auto &reward : rewards) {
                reward.draw(window);
            }
            ufo.draw(window);
        } else if (currentScreen == ScreenType::Ende) {
            // Rysowanie ekranu końca gry
            window.clear(sf::Color::Black);
            window.draw(endeText);
        } else if (currentScreen == ScreenType::Los) {
            // Rysowanie ekranu menu
            window.clear(sf::Color::Black);
            window.draw(losText);
        }
    }
};


int main() {
    try {
        // Kontener do przechowywania danych gry
        std::vector<GameData> gameDataList;
        // Inicjalizacja generatora liczb losowych
        srand(static_cast<unsigned int>(time(nullptr)));

        // Tworzenie okna gry o określonych wymiarach
        sf::Vector2f windowSize(1200, 750);
        sf::RenderWindow window(sf::VideoMode(static_cast<unsigned int>(windowSize.x), static_cast<unsigned int>(windowSize.y)), "Space Game");

        // Inicjalizacja interfejsu z rozmiarem okna i ustawienie początkowego tekstu pozycji
        Interfejs interfejs(windowSize);
        interfejs.setText("Pozycja: (0, 0)", 10);

        // Pobranie centralnych granic dla obszaru gry i utworzenie UFO na środku
        sf::FloatRect centralBounds = interfejs.getCentralBounds();
        Ufo ufo(centralBounds.left + centralBounds.width / 2 - 25.f, centralBounds.top + centralBounds.height / 2 - 25.f);

        // Inicjalizacja wyniku
        int score = 0;

        // Próba wczytania zapisanych danych gry z pliku JSON
        try {
            sf::Vector2f loadedPosition = ufo.getPosition();
            loadScoreFromJson("sscore.json", gameDataList, loadedPosition, score);
            ufo.setPosition(loadedPosition);
            std::cout << "Dane gry załadowane z pliku JSON." << std::endl;
        } catch (const std::exception &e) {
            std::cerr << "Błąd ładowania danych gry: " << e.what() << ". Gra rozpocznie się z domyślnymi ustawieniami." << std::endl;
        }

        // Definicja poziomów gry z różnymi właściwościami
        std::vector<Level> levels = {
            {sf::Color::Black, 100.f, 5},    // Poziom 1: Czarne tło, wolna prędkość, mało przeszkód
            {sf::Color::Cyan, 150.f, 8},     // Poziom 2: Cyjanowe tło, średnia prędkość, więcej przeszkód
            {sf::Color::Magenta, 215.f, 10}, // Poziom 3: Magenta tło, duża prędkość, dużo przeszkód
        };
        int currentLevelIndex = 0;
        Level currentLevel = levels[currentLevelIndex];

        // Funkcja lambda do inicjalizacji przeszkód dla aktualnego poziomu
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

        // Funkcja lambda do inicjalizacji nagród
        std::vector<Reward> rewards;
        auto initializeRewards = [&]() {
            rewards.clear();
            for (int i = 0; i < 3; ++i) {
                float x = centralBounds.left + static_cast<float>(rand() % static_cast<int>(centralBounds.width));
                float y = centralBounds.top + static_cast<float>(rand() % static_cast<int>(centralBounds.height));
                rewards.emplace_back(x, y, currentLevel.obstacleSpeed / 2);
            }
        };
        initializeRewards();

        // Zmienne stanu gry
        bool isGameOver = false;
        sf::Clock clock;

        // Inicjalizacja menedżera ekranów dla różnych widoków gry
        ScreenManager screenManager(windowSize);

        // Główna pętla gry
        while (window.isOpen()) {
            // Obsługa zdarzeń
            sf::Event event;
            while (window.pollEvent(event)) {
                if (event.type == sf::Event::Closed)
                    window.close();

                // Obsługa wejścia z klawiatury
                if (event.type == sf::Event::KeyPressed) {
                    // Klawisz M - Przełączanie między ekranem Los a Game
                    if (event.key.code == sf::Keyboard::M) {
                        if (screenManager.getCurrentScreen() == ScreenManager::ScreenType::Los) {
                            screenManager.switchTo(ScreenManager::ScreenType::Game);
                        } else {
                            screenManager.switchTo(ScreenManager::ScreenType::Los);
                        }
                    }
                    // Klawisz G - Restart gry z ekranu Ende
                    else if (event.key.code == sf::Keyboard::G) {
                        if (screenManager.getCurrentScreen() == ScreenManager::ScreenType::Ende) {
                            isGameOver = false;
                            try {
                                sf::Vector2f ufoPosition = ufo.getPosition();
                                loadScoreFromJson("sscore.json", gameDataList, ufoPosition, score);
                                ufo.setPosition(ufoPosition);
                                interfejs.updateTexts(ufo.getPosition(), score);
                                std::cout << "Dane gry zostały załadowane z pliku JSON." << std::endl;
                            } catch (const std::exception &e) {
                                std::cerr << "Błąd ładowania danych gry: " << e.what() << ". Gra rozpocznie się z domyślnymi ustawieniami." << std::endl;
                            }
                            initializeObstacles();
                            initializeRewards();
                            screenManager.switchTo(ScreenManager::ScreenType::Game);
                        }
                    }
                    // Klawisz Escape - Obsługa pauzy i wyjścia z gry
                    else if (event.key.code == sf::Keyboard::Escape) {
                        if (interfejs.isPauseVisible()) {
                            // Zapisywanie danych gry przed wyjściem
                            GameData newGameData;
                            newGameData.position = ufo.getPosition();
                            newGameData.score = score;

                            // Formatowanie i zapisywanie aktualnej daty
                            auto now = std::time(nullptr);
                            char buf[80];
                            std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                            newGameData.date = buf;

                            gameDataList.push_back(newGameData);
                            saveScoreToJson("sscore.json", gameDataList);

                            std::cout << "Dane gry zostały zapisane do pliku 'score.json'." << std::endl;

                            interfejs.requestExit();
                        } else {
                            interfejs.togglePause();
                        }
                    }
                    // Klawisz Shift - Wznowienie gry z pauzy
                    else if (event.key.code == sf::Keyboard::LShift || event.key.code == sf::Keyboard::RShift) {
                        if (interfejs.isPauseVisible()) {
                            interfejs.resumeGame();
                        }
                    }
                    // Klawisz F1 - Przełączanie ekranu pomocy
                    else if (event.key.code == sf::Keyboard::F1) {
                        interfejs.toggleHelp();
                    }
                    // Klawisz Enter - Zmiana poziomu
                    else if (event.key.code == sf::Keyboard::Return) {
                        currentLevelIndex = (currentLevelIndex + 1) % levels.size();
                        currentLevel = levels[currentLevelIndex];
                        initializeObstacles();
                        initializeRewards();
                        std::cout << "Poziom zmieniony na: " << currentLevelIndex + 1 << std::endl;
                    }
                    // Klawisz F - Wczytanie danych gry
                    else if (event.key.code == sf::Keyboard::F) {
                        sf::Vector2f ufoPosition = ufo.getPosition();
                        loadScoreFromJson("sscore.json", gameDataList, ufoPosition, score);
                        ufo.setPosition(ufoPosition);
                        interfejs.updateTexts(ufo.getPosition(), score);
                        std::cout << "Dane gry zostały ponownie załadowane." << std::endl;
                    }
                    // Klawisz S - Zapisanie danych gry
                    else if (event.key.code == sf::Keyboard::S) {
                        GameData newGameData;
                        newGameData.position = ufo.getPosition();
                        newGameData.score = score;

                        auto now = std::time(nullptr);
                        char buf[80];
                        std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
                        newGameData.date = buf;

                        gameDataList.push_back(newGameData);
                        saveScoreToJson("sscore.json", gameDataList);

                        std::cout << "Dane gry zostały zapisane do pliku 'score.json'." << std::endl;
                    }
                }
            }

            // Obsługa wyjścia z gry
            if (interfejs.isExitRequested()) {
                saveScoreToJson("sscore.json", gameDataList);
                window.close();
            }

            // Aktualizacja czasu gry
            float deltaTime = clock.restart().asSeconds();

            // Aktualizacja logiki gry gdy jesteśmy na ekranie Game
            if (screenManager.getCurrentScreen() == ScreenManager::ScreenType::Game) {
                if (!interfejs.isHelpVisible() && !interfejs.isPauseVisible() && !isGameOver) {
                    // Aktualizacja pozycji UFO
                    ufo.update(deltaTime, centralBounds);

                    // Sprawdzanie kolizji z przeszkodami
                    for (auto &obstacle : obstacles) {
                        obstacle.update(deltaTime, centralBounds);

                        if (ufo.getBounds().intersects(obstacle.getBounds())) {
                            static sf::Clock collisionClock;
                            if (collisionClock.getElapsedTime().asMilliseconds() > 500) {
                                score -= 1;
                                collisionClock.restart();
                                if (score < 0) {
                                    screenManager.switchTo(ScreenManager::ScreenType::Ende);
                                    isGameOver = true;
                                    interfejs.showGameOver();
                                    saveScoreToJson("sscore.json", gameDataList);
                                }
                            }
                        }
                    }

                    // Sprawdzanie kolizji z nagrodami
                    for (auto it = rewards.begin(); it != rewards.end();) {
                        it->update(deltaTime, centralBounds);

                        if (ufo.getBounds().intersects(it->getBounds())) {
                            score += 1;
                            it = rewards.erase(it);
                        } else {
                            ++it;
                        }
                    }

                    // Aktualizacja tekstu interfejsu
                    interfejs.updateTexts(ufo.getPosition(), score);
                }
            }

            // Renderowanie gry
            window.clear(currentLevel.backgroundColor);
            screenManager.draw(window, interfejs, ufo, obstacles, rewards);
            window.display();
        }

    } catch (const std::exception &e) {
        // Obsługa nieoczekiwanych wyjątków
        std::cerr << "Wyjątek: " << e.what() << std::endl;
    }

    return 0;
}