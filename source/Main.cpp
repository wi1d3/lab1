#include <vector>
#include <algorithm>
#include <functional> 
#include <memory>
#include <cstdlib>
#include <cmath>
#include <ctime>

#include <raylib.h>
#include <raymath.h>

// --- UTILS ---
namespace Utils {
	inline static float RandomFloat(float min, float max) {
		return min + static_cast<float>(rand()) / RAND_MAX * (max - min);
	}
}

// --- TRANSFORM, PHYSICS, LIFETIME, RENDERABLE ---
struct TransformA {
	Vector2 position{};
	float rotation{};
};

struct Physics {
	Vector2 velocity{};
	float rotationSpeed{};
};

struct Renderable {
	enum Size { SMALL = 1, MEDIUM = 2, LARGE = 4 } size = SMALL;
};

// --- RENDERER ---
class Renderer {
public:
	static Renderer& Instance() {
		static Renderer inst;
		return inst;
	}

	void Init(int w, int h, const char* title) {
		InitWindow(w, h, title);
		SetTargetFPS(60);
		screenW = w;
		screenH = h;
	}

	void Begin() {
		BeginDrawing();
		ClearBackground(BLACK);
	}

	void End() {
		EndDrawing();
	}

	void DrawPoly(const Vector2& pos, int sides, float radius, float rot) {
		DrawPolyLines(pos, sides, radius, rot, BLACK);
	}

	int Width() const {
		return screenW;
	}

	int Height() const {
		return screenH;
	}

private:
	Renderer() = default;

	int screenW{};
	int screenH{};
};

// --- ASTEROID HIERARCHY ---

class Asteroid {
public:
	Asteroid(int screenW, int screenH) {
		init(screenW, screenH);
	}
	virtual ~Asteroid() = default;

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		transform.rotation += physics.rotationSpeed * dt;
		if (transform.position.x < -GetRadius() || transform.position.x > Renderer::Instance().Width() + GetRadius() ||
			transform.position.y < -GetRadius() || transform.position.y > Renderer::Instance().Height() + GetRadius())
			return false;
		return true;
	}
	virtual void Draw() const = 0;

	Vector2 GetPosition() const {
		return transform.position;
	}

	float constexpr GetRadius() const {
		return 16.f * (float)render.size;
	}

	int GetDamage() const {
		return baseDamage * static_cast<int>(render.size);
	}

	int GetSize() const {
		return static_cast<int>(render.size);
	}

protected:
	void init(int screenW, int screenH, bool nightmare = false) {
		// Choose size
		render.size = static_cast<Renderable::Size>(1 << GetRandomValue(0, 2));

		// Spawn at random edge
		switch (GetRandomValue(0, 3)) {
		case 0:
			transform.position = { Utils::RandomFloat(0, screenW), -GetRadius() };
			break;
		case 1:
			transform.position = { screenW + GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		case 2:
			transform.position = { Utils::RandomFloat(0, screenW), screenH + GetRadius() };
			break;
		default:
			transform.position = { -GetRadius(), Utils::RandomFloat(0, screenH) };
			break;
		}

		// Aim towards center with jitter
		float maxOff = fminf(screenW, screenH) * 0.1f;
		float ang = Utils::RandomFloat(0, 2 * PI);
		float rad = Utils::RandomFloat(0, maxOff);
		Vector2 center = {
										 screenW * 0.5f + cosf(ang) * rad,
										 screenH * 0.5f + sinf(ang) * rad
		};

		Vector2 dir = Vector2Normalize(Vector2Subtract(center, transform.position));
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(SPEED_MIN, SPEED_MAX));
		physics.rotationSpeed = Utils::RandomFloat(ROT_MIN, ROT_MAX);

		transform.rotation = Utils::RandomFloat(0, 360);

		float speedMin = nightmare ? SPEED_MIN * 1.5f : SPEED_MIN;
		float speedMax = nightmare ? SPEED_MAX * 1.5f : SPEED_MAX;
		physics.velocity = Vector2Scale(dir, Utils::RandomFloat(speedMin, speedMax));
	}

	TransformA transform;
	Physics    physics;
	Renderable render;

	int baseDamage = 0;
	static constexpr float LIFE = 10.f;
	static constexpr float SPEED_MIN = 125.f;
	static constexpr float SPEED_MAX = 250.f;
	static constexpr float ROT_MIN = 50.f;
	static constexpr float ROT_MAX = 240.f;
};

class TriangleAsteroid : public Asteroid {
public:
	TriangleAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 3, GetRadius(), transform.rotation);
	}
};
class SquareAsteroid : public Asteroid {
public:
	SquareAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 10; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 4, GetRadius(), transform.rotation);
	}
};
class PentagonAsteroid : public Asteroid {
public:
	PentagonAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 15; }
	void Draw() const override {
		Renderer::Instance().DrawPoly(transform.position, 5, GetRadius(), transform.rotation);
	}
};
void DrawHeart(Vector2 center, float size, float rotation) {
	const int segments = 100;
	Vector2 points[segments];
	for (int i = 0; i < segments; ++i) {
		float t = i * 2 * PI / segments;
		float x = 16 * powf(sinf(t), 3);
		float y = 13 * cosf(t) - 5 * cosf(2 * t) - 2 * cosf(3 * t) - cosf(4 * t);
		x *= size / 32.0f;
		y *= size / 32.0f;
		float angle = rotation * DEG2RAD;
		float rx = x * cosf(angle) - y * sinf(angle);
		float ry = x * sinf(angle) + y * cosf(angle);
		points[i] = { center.x + rx, center.y - ry };
	}

	for (int i = 0; i < segments - 1; ++i) {
		DrawLineV(points[i], points[i + 1], RED);
	}
	DrawLineV(points[segments - 1], points[0], RED);
}

void DrawStar(Vector2 center, float radius, float rotation) {
	const int points = 10; // 5 ramion * 2 (zewnętrzne i wewnętrzne)
	Vector2 star[points];
	float angleStep = 2 * PI / points;
	for (int i = 0; i < points; ++i) {
		float r = (i % 2 == 0) ? radius : radius * 0.5f;
		float angle = i * angleStep + rotation * DEG2RAD;
		star[i] = {
			center.x + r * cosf(angle),
			center.y + r * sinf(angle)
		};
	}
	for (int i = 0; i < points; ++i) {
		DrawLineV(star[i], star[(i + 1) % points], YELLOW);
	}
}
void DrawFlower(Vector2 center, float radius, float rotation) {
	const int segments = 100;
	Vector2 points[segments];
	for (int i = 0; i < segments; ++i) {
		float t = i * 2 * PI / segments;
		float r = radius * (1 + 0.3f * sinf(6 * t));
		float x = r * cosf(t);
		float y = r * sinf(t);
		float angle = rotation * DEG2RAD;
		float rx = x * cosf(angle) - y * sinf(angle);
		float ry = x * sinf(angle) + y * cosf(angle);
		points[i] = { center.x + rx, center.y + ry };
	}
	for (int i = 0; i < segments - 1; ++i) {
		DrawLineV(points[i], points[i + 1], MAGENTA);
	}
	DrawLineV(points[segments - 1], points[0], MAGENTA);
}


class HeartShapeAsteroid : public Asteroid {
public:
	HeartShapeAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		DrawHeart(transform.position, GetRadius(), transform.rotation);
	}
};

class StarShapeAsteroid : public Asteroid {
public:
	StarShapeAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		DrawStar(transform.position, GetRadius(), transform.rotation); // 10-bok gwiazdka
	}
};

class FlowerAsteroid : public Asteroid {
public:
	FlowerAsteroid(int w, int h) : Asteroid(w, h) { baseDamage = 5; }
	void Draw() const override {
		DrawFlower(transform.position, GetRadius(), transform.rotation); // 8-bok = kwiatek
	}
};
// Shape selector
enum class AsteroidShape { TRIANGLE = 3, SQUARE = 4, PENTAGON = 5, RANDOM = 0 };

// Factory
static inline std::unique_ptr<Asteroid> MakeAsteroid(int w, int h, AsteroidShape shape, bool nightmare = false) {
	if (!nightmare) {
		int r = GetRandomValue(0, 2);
		switch (r) {
		case 0: return std::make_unique<HeartShapeAsteroid>(w, h);
		case 1: return std::make_unique<StarShapeAsteroid>(w, h);
		default: return std::make_unique<FlowerAsteroid>(w, h);
		}
	}

	// Nightmare – klasyczne kształty
	switch (shape) {
	case AsteroidShape::TRIANGLE: return std::make_unique<TriangleAsteroid>(w, h);
	case AsteroidShape::SQUARE: return std::make_unique<SquareAsteroid>(w, h);
	case AsteroidShape::PENTAGON: return std::make_unique<PentagonAsteroid>(w, h);
	default:
		return MakeAsteroid(w, h, static_cast<AsteroidShape>(3 + GetRandomValue(0, 2)), nightmare);
	}
}


// --- PROJECTILE HIERARCHY ---
enum class WeaponType { LASER, BULLET, COUNT };
class Projectile {
public:
	Projectile(Vector2 pos, Vector2 vel, int dmg, WeaponType wt, bool nm = false)
		: nightmare(nm)
	{
		transform.position = pos;
		physics.velocity = vel;
		baseDamage = dmg;
		type = wt;
	}

	static void LoadAssets() {
		if (!starLoaded) {
			starTexture = LoadTexture("gwiazda.png");
			starTextureNightmare = LoadTexture("blyskawica.png");
			GenTextureMipmaps(&starTexture);
			GenTextureMipmaps(&starTextureNightmare);
			SetTextureFilter(starTexture, TEXTURE_FILTER_BILINEAR);
			SetTextureFilter(starTextureNightmare, TEXTURE_FILTER_BILINEAR);
			starLoaded = true;
		}
	}

	static void UnloadAssets() {
		if (starLoaded) {
			UnloadTexture(starTexture);
			UnloadTexture(starTextureNightmare);
			starLoaded = false;
		}
	}

	bool Update(float dt) {
		transform.position = Vector2Add(transform.position, Vector2Scale(physics.velocity, dt));
		return transform.position.x < 0 || transform.position.x > Renderer::Instance().Width() ||
			transform.position.y < 0 || transform.position.y > Renderer::Instance().Height();
	}

	void Draw() const {
		if (type == WeaponType::BULLET) {
			if (starLoaded) {
				Texture2D tex = nightmare ? starTextureNightmare : starTexture;
				Vector2 drawPos = {
					transform.position.x - (tex.width * BULLET_SCALE) / 2.0f,
					transform.position.y - (tex.height * BULLET_SCALE) / 2.0f
				};
				DrawTextureEx(tex, drawPos, 0.0f, BULLET_SCALE, WHITE);
			}
		}

		else {
	
			static constexpr float LASER_LENGTH = 30.f;
			Rectangle lr = { transform.position.x - 2.f, transform.position.y - LASER_LENGTH, 4.f, LASER_LENGTH };
			float t = GetTime() * 2.0f;
			Color rainbow = nightmare ? RED : Color{
				(unsigned char)((sinf(t + 0.f) * 0.5f + 0.5f) * 255),
				(unsigned char)((sinf(t + 2.f) * 0.5f + 0.5f) * 255),
				(unsigned char)((sinf(t + 4.f) * 0.5f + 0.5f) * 255),
				255
			};
			float len = nightmare ? LASER_LENGTH * 1.5f : LASER_LENGTH;

			DrawRectangleRec(lr, rainbow);
		}
	}

	Vector2 GetPosition() const { return transform.position; }

	float GetRadius() const {
		if (type == WeaponType::BULLET) {
			return (starTexture.width * BULLET_SCALE) / 2.f;
		}
		else {
			return 2.f;
		}
	}

	int GetDamage() const { return baseDamage; }

private:
	TransformA transform;
	Physics    physics;
	int        baseDamage;
	WeaponType type;

	inline static Texture2D starTexture;
	inline static bool starLoaded = false;
	inline static constexpr float BULLET_SCALE = 0.06f;
	inline static Texture2D starTextureNightmare;
	inline static bool nightmareAssetsLoaded = false;
	bool nightmare = false;


};

inline static Projectile MakeProjectile(WeaponType wt,
	const Vector2 pos,
	float speed, bool nightmare = false)
{
	Vector2 vel{ 0, -speed };
	if (wt == WeaponType::LASER) {
		return Projectile(pos, vel, 20, wt, nightmare);
	}
	else {
		return Projectile(pos, vel, 10, wt, nightmare);
	}
}

// --- SHIP HIERARCHY ---
class Ship {
public:
	Ship(int screenW, int screenH) {
		transform.position = {
			screenW * 0.5f,
			screenH * 0.5f
		};
		hp = 100;
		speed = 250.f;
		alive = true;

		// per-weapon fire rate & spacing
		fireRateLaser = 18.f; // shots/sec
		fireRateBullet = 22.f;
		spacingLaser = 40.f; // px between lasers
		spacingBullet = 20.f;
	}
	virtual ~Ship() = default;
	virtual void Update(float dt) = 0;
	virtual void Draw() const = 0;

	void TakeDamage(int dmg) {
		if (!alive) return;
		hp -= dmg;
		if (hp <= 0) alive = false;
	}

	bool IsAlive() const {
		return alive;
	}

	Vector2 GetPosition() const {
		return transform.position;
	}

	virtual float GetRadius() const = 0;

	int GetHP() const {
		return hp;
	}

	float GetFireRate(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? fireRateLaser : fireRateBullet;
	}

	float GetSpacing(WeaponType wt) const {
		return (wt == WeaponType::LASER) ? spacingLaser : spacingBullet;
	}

protected:
	TransformA transform;
	int        hp;
	float      speed;
	bool       alive;
	float      fireRateLaser;
	float      fireRateBullet;
	float      spacingLaser;
	float      spacingBullet;
};

class PlayerShip :public Ship {
public:
	PlayerShip(int w, int h) : Ship(w, h) {
		texture = LoadTexture("unicorn.png");
		nightmareTexture = LoadTexture("unicorn_nightmare.png");
		GenTextureMipmaps(&texture);                                                        // Generate GPU mipmaps for a texture
		GenTextureMipmaps(&nightmareTexture);
		SetTextureFilter(texture, 2);
		SetTextureFilter(nightmareTexture, TEXTURE_FILTER_BILINEAR);
		scale = 0.08f;
	}
	~PlayerShip() {
		UnloadTexture(texture);
		UnloadTexture(nightmareTexture);
	}

	void Update(float dt) override {
		if (alive) {
			if (IsKeyDown(KEY_W)) transform.position.y -= speed * dt;
			if (IsKeyDown(KEY_S)) transform.position.y += speed * dt;
			if (IsKeyDown(KEY_A)) transform.position.x -= speed * dt;
			if (IsKeyDown(KEY_D)) transform.position.x += speed * dt;
		}
		else {
			transform.position.y += speed * dt;
		}
	}

	void EnableNightmareMode() {
		useNightmareTexture = true;
	}

	void Draw() const override {
		if (!alive && fmodf(GetTime(), 0.4f) > 0.2f) return;
		Texture2D tex = useNightmareTexture ? nightmareTexture : texture;
		Vector2 dstPos = {
										 transform.position.x - (texture.width * scale) * 0.5f,
										 transform.position.y - (texture.height * scale) * 0.5f
		};
		if (useNightmareTexture) DrawTextureEx(tex, dstPos, 0.0f, 0.4f, WHITE);
		else DrawTextureEx(tex, dstPos, 0.0f, scale, WHITE);
		
	}

	float GetRadius() const override {
		float res = 0;
		if (useNightmareTexture) res=(texture.width * scale) * 0.5f;
		else  res = (texture.width * scale) * 0.5f;
		return res;
	}

private:
	Texture2D texture;
	float     scale;
	Texture2D nightmareTexture;
	bool useNightmareTexture = false;
};

class Heart {
public:
	Heart(int screenW, int screenH) {
		position = { Utils::RandomFloat(50, screenW - 50), -30 }; 
		velocity = { 0, 100.0f };
	}

	static void LoadAssets() {
		if (!loaded) {
			heartTex = LoadTexture("cake.png");
			heartTexNightmare = LoadTexture("heart.png");
			SetTextureFilter(heartTex, TEXTURE_FILTER_BILINEAR);
			SetTextureFilter(heartTexNightmare, TEXTURE_FILTER_BILINEAR);
			loaded = true;
		}
	}

	static void UnloadAssets() {
		if (loaded) {
			UnloadTexture(heartTex);
			UnloadTexture(heartTexNightmare);
			loaded = false;
		}
	}

	bool Update(float dt) {
		position = Vector2Add(position, Vector2Scale(velocity, dt));
		return position.y > Renderer::Instance().Height();
	}

	void Draw(bool nightmare) const {
		float usedScale = nightmare ? scale : scale * 1.4f;
		Texture2D tex = nightmare ? heartTexNightmare : heartTex;
		Vector2 drawPos = { position.x - tex.width / 2.0f * usedScale, position.y - tex.height / 2.0f * usedScale };
		DrawTextureEx(tex, drawPos, 0.0f, usedScale, WHITE);

	}

	Vector2 GetPosition() const { return position; }
	float GetRadius() const { return (heartTex.width * scale) / 2.0f; }

private:
	Vector2 position;
	Vector2 velocity;
	inline static Texture2D heartTex;
	inline static Texture2D heartTexNightmare;
	inline static bool loaded = false;
	static constexpr float scale = 0.07f;
};

// --- APPLICATION ---
class Application {
public:
	static Application& Instance() {
		static Application inst;
		return inst;
	}
	
	void Run() {
		bool paused = false;
		srand(static_cast<unsigned>(time(nullptr)));
		Renderer::Instance().Init(C_WIDTH, C_HEIGHT, "Unicorns OOP");
		Projectile::LoadAssets();
		Heart::LoadAssets();
		auto player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);

		float spawnTimer = 0.f;
		float spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
		WeaponType currentWeapon = WeaponType::LASER;
		float shotTimer = 0.f;

		bool nightmareMode = false;

		float boostCooldown = 0.0f; 
		
		while (!WindowShouldClose()) {
			float dt = GetFrameTime();
			spawnTimer += dt;

			if (IsKeyPressed(KEY_P)) {
				paused = !paused;
			}
			if (!paused)
			{
				if (!nightmareMode && score >= 200) {
					nightmareMode = true;
					player->EnableNightmareMode();
				}

				// Update player
				player->Update(dt);

				heartSpawnTimer += dt;
				if (heartSpawnTimer >= heartSpawnInterval) {
					hearts.emplace_back(C_WIDTH, C_HEIGHT);
					heartSpawnTimer = 0.0f;
					heartSpawnInterval = Utils::RandomFloat(12.0f, 15.0f); 				}

				//kolizja serc z graczem
				auto heart_it = hearts.begin();
				while (heart_it != hearts.end()) {
					if (heart_it->Update(dt)) {
						heart_it = hearts.erase(heart_it); // wypadło poza ekran
						continue;
					}

					float dist = Vector2Distance(player->GetPosition(), heart_it->GetPosition());
					if (dist < player->GetRadius() + heart_it->GetRadius()) {
						if (player->IsAlive() && player->GetHP() < 100) {
							int missing = 100 - player->GetHP();
							player->TakeDamage(-std::min(40, missing)); // lecz tylko brakujące
						}

						heart_it = hearts.erase(heart_it);
					}
					else {
						++heart_it;
					}
				}

				// Power Boost: usuń wszystkie asteroidy
				if (IsKeyPressed(KEY_J) && powerBoostAvailable) {
					flashActive = true;
					flashTimer = 0.2f;
					asteroids.clear();
					powerBoostAvailable = false;
					boostCharge = 0.0f; 
				}

				// Restart logic
				if (!player->IsAlive() && IsKeyPressed(KEY_R)) {
					player = std::make_unique<PlayerShip>(C_WIDTH, C_HEIGHT);
					score = 0;
					boostCharge = 0.0f;
					nightmareMode = false;
					asteroids.clear();
					projectiles.clear();
					spawnTimer = 0.f;
					spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
				}
				// Asteroid shape switch
				if (IsKeyPressed(KEY_ONE)) {
					currentShape = AsteroidShape::TRIANGLE;
				}
				if (IsKeyPressed(KEY_TWO)) {
					currentShape = AsteroidShape::SQUARE;
				}
				if (IsKeyPressed(KEY_THREE)) {
					currentShape = AsteroidShape::PENTAGON;
				}
				if (IsKeyPressed(KEY_FOUR)) {
					currentShape = AsteroidShape::RANDOM;
				}


				// Weapon switch
				if (IsKeyPressed(KEY_TAB)) {
					currentWeapon = static_cast<WeaponType>((static_cast<int>(currentWeapon) + 1) % static_cast<int>(WeaponType::COUNT));
				}

				// Shooting
				{
					if (player->IsAlive() && IsKeyDown(KEY_SPACE)) {
						shotTimer += dt;
						float interval = 1.f / player->GetFireRate(currentWeapon);
						float projSpeed = player->GetSpacing(currentWeapon) * player->GetFireRate(currentWeapon);

						while (shotTimer >= interval) {
							Vector2 p = player->GetPosition();
							p.y -= player->GetRadius();
							projectiles.push_back(MakeProjectile(currentWeapon, p, projSpeed, nightmareMode));
							shotTimer -= interval;
						}
					}
					else {
						float maxInterval = 1.f / player->GetFireRate(currentWeapon);

						if (shotTimer > maxInterval) {
							shotTimer = fmodf(shotTimer, maxInterval);
						}
					}
				}

				// Spawn asteroids
				if (spawnTimer >= spawnInterval && asteroids.size() < MAX_AST) {
					asteroids.push_back(MakeAsteroid(C_WIDTH, C_HEIGHT, currentShape, nightmareMode));
					spawnTimer = 0.f;
					spawnInterval = Utils::RandomFloat(C_SPAWN_MIN, C_SPAWN_MAX);
				}

				if (nightmareMode) {
					spawnInterval = Utils::RandomFloat(C_SPAWN_MIN * 0.5f, C_SPAWN_MAX * 0.5f);
				}

				// Update projectiles - check if in boundries and move them forward
				{
					auto projectile_to_remove = std::remove_if(projectiles.begin(), projectiles.end(),
						[dt](auto& projectile) {
							return projectile.Update(dt);
						});
					projectiles.erase(projectile_to_remove, projectiles.end());
				}

				// Projectile-Asteroid collisions O(n^2)
				for (auto pit = projectiles.begin(); pit != projectiles.end();) {
					bool removed = false;

					for (auto ait = asteroids.begin(); ait != asteroids.end(); ++ait) {
						float dist = Vector2Distance((*pit).GetPosition(), (*ait)->GetPosition());
						if (dist < (*pit).GetRadius() + (*ait)->GetRadius()) {
							score += (*ait)->GetSize() * 10;
							boostCharge += (*ait)->GetSize() * 10.0f / 300.0f;
							if (boostCharge >= 1.0f) {
								boostCharge = 1.0f;
								powerBoostAvailable = true;
							}

							ait = asteroids.erase(ait);
							pit = projectiles.erase(pit);
							removed = true;
							break;
						}
					}
					if (!removed) {
						++pit;
					}
				}

				// Asteroid-Ship collisions
				{
					auto remove_collision =
						[&player, dt](auto& asteroid_ptr_like) -> bool {
						if (player->IsAlive()) {
							float dist = Vector2Distance(player->GetPosition(), asteroid_ptr_like->GetPosition());

							if (dist < player->GetRadius() + asteroid_ptr_like->GetRadius()) {
								player->TakeDamage(asteroid_ptr_like->GetDamage());
								return true; // Mark asteroid for removal due to collision
							}
						}
						if (!asteroid_ptr_like->Update(dt)) {
							return true;
						}
						return false; // Keep the asteroid
						};
					auto asteroid_to_remove = std::remove_if(asteroids.begin(), asteroids.end(), remove_collision);
					asteroids.erase(asteroid_to_remove, asteroids.end());
				}
			}

			// Render everything
			{
				Renderer::Instance().Begin();
				if (flashActive) {
					flashTimer -= GetFrameTime();
					if (flashTimer <= 0.0f) {
						flashActive = false;
					}
					else {
						DrawRectangle(0, 0, C_WIDTH, C_HEIGHT, WHITE); // pełny biały ekran
					}
				}

				for (const auto& heart : hearts) {
					heart.Draw(nightmareMode);
				}

				if (nightmareMode) {
					ClearBackground(DARKGRAY);
					float flashAlpha = (sinf(GetTime() * 10) * 0.5f + 0.5f) * 0.3f;
					DrawRectangle(0, 0, C_WIDTH, C_HEIGHT, Fade(RED, flashAlpha));
		
					if (fmodf(GetTime(), 1.0f) < 0.5f) {
						const char* nightmareText = "NIGHTMARE MODE";
						int textWidth = MeasureText(nightmareText, 40);
						DrawText(nightmareText,
							(C_WIDTH - textWidth) / 2,
							100,
							40,
							RED);
					}
				}
				else {
					float t = GetTime() * 0.5f;
					Color bg = {
						(unsigned char)(150 + 50 * sinf(t)),
						(unsigned char)(200 + 50 * sinf(t + 2)),
						(unsigned char)(230 + 25 * sinf(t + 4)),
						255
					};
					ClearBackground(bg);
				}
				if(nightmareMode) DrawText(TextFormat("HP: %d", player->GetHP()),10, 10, 20, GREEN);
				else DrawText(TextFormat("BEAUTY: %d", player->GetHP()),10, 10, 20, PINK);

				if (!player->IsAlive()) {
					DrawText("GAME OVER", C_WIDTH / 2 - MeasureText("GAME OVER", 40) / 2, C_HEIGHT / 2 - 40, 40, RED);
					DrawText("Press R to restart", C_WIDTH / 2 - MeasureText("Press R to restart", 20) / 2, C_HEIGHT / 2 + 10, 20, DARKGRAY);
					DrawText(TextFormat("Score: %d", score), C_WIDTH / 2 - MeasureText(TextFormat("Score: %d", score), 20) / 2, C_HEIGHT / 2 + 40, 20, BLACK);

				}
				const char* weaponName;
				if(nightmareMode) weaponName = (currentWeapon == WeaponType::LASER) ? "DEATH" : "TREMOR";
				else weaponName = (currentWeapon == WeaponType::LASER) ? "LOVE" : "FRIENDSHIP";
				DrawText(TextFormat("Power: %s", weaponName),
					10, 40, 20, BLUE);

				DrawText(TextFormat("Score: %d", score), 10, 70, 20, YELLOW);

				DrawText("Power Boost", 10, 130, 20, RAYWHITE);
				DrawRectangle(10, 160, 200, 20, GRAY); // tło paska
				DrawRectangle(10, 160, (int)(200 * boostCharge), 20, RED); // poziom naładowania

				if (powerBoostAvailable) {
					DrawText("PRESS J TO UNLEASH!", 10, 190, 20, YELLOW);
				}
		
				for (const auto& projPtr : projectiles) {
					projPtr.Draw();
				}
				for (const auto& astPtr : asteroids) {
					astPtr->Draw();
				}

				player->Draw();

				if (paused) {
					DrawRectangle(0, 0, Renderer::Instance().Width(), Renderer::Instance().Height(), Fade(BLACK, 0.5f));
					DrawText("PAUSED", C_WIDTH / 2 - 50, C_HEIGHT / 2, 40, RAYWHITE);
				}
				Renderer::Instance().End();
			}
		}
		Heart::UnloadAssets();
		Projectile::UnloadAssets();
	}

private:
	Application()
	{
		asteroids.reserve(1000);
		projectiles.reserve(10'000);
	};

	std::vector<std::unique_ptr<Asteroid>> asteroids;
	std::vector<Projectile> projectiles;

	AsteroidShape currentShape = AsteroidShape::TRIANGLE;

	static constexpr int C_WIDTH = 1200;
	static constexpr int C_HEIGHT = 1200;
	static constexpr size_t MAX_AST = 150;
	static constexpr float C_SPAWN_MIN = 0.5f;
	static constexpr float C_SPAWN_MAX = 3.0f;

	static constexpr int C_MAX_ASTEROIDS = 1000;
	static constexpr int C_MAX_PROJECTILES = 10'000;
	int score = 0;
	bool powerBoostAvailable = false;
	bool flashActive = false;
	float flashTimer = 0.0f;
	float boostCharge = 0.0f;

	std::vector<Heart> hearts;
	float heartSpawnTimer = 0.0f;
	float heartSpawnInterval = Utils::RandomFloat(12.0f, 15.0f);

};

int main() {
	Application::Instance().Run();
	return 0;
}
