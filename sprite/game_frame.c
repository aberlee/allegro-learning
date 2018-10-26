#include <stdio.h>
#include <math.h>
#include <allegro5/allegro.h>
#include <allegro5/allegro_image.h>

#include "sprite.c"

#define FPS	60

#define SPRITE_FILE  "character.json"

#define BG_WIDTH		640
#define BG_HEIGHT		480
#define BG_COLOR	(al_map_rgb(0, 0, 0))

enum {
	STAY = 0,
	WALK,
	JUMP,
	ATTACK,
	JUMP_ATTACK,
};

static ALLEGRO_DISPLAY *display = NULL;
static ALLEGRO_EVENT_QUEUE *eventq = NULL;
static ALLEGRO_TIMER *timer = NULL;
static ALLEGRO_SPRITE *sprite = NULL;
static bool running = true;
static bool redraw = true;

static int game_init_map(void)
{
	return 0;
}

static int game_init_sprite(void)
{
	sprite = al_load_sprite(SPRITE_FILE);
	if (!sprite) {
		fprintf(stderr, "failed to load sprite "SPRITE_FILE"!\n");
		return -1;
	}

	al_sprite_set_map_size(sprite, BG_WIDTH, BG_HEIGHT);
	al_sprite_move_to(sprite, BG_WIDTH/2, BG_HEIGHT/2);

	al_sprite_add_action(sprite, STAY, 0, 2, 20, true);
	al_sprite_add_action(sprite, WALK, 1, 4, 10, true);
	al_sprite_add_action(sprite, JUMP, 2, 4, 10, false);
	al_sprite_add_action(sprite, ATTACK, 4, 4, 10, false);
	al_sprite_add_action(sprite, JUMP_ATTACK, 4, 4, 10, false);

	al_sprite_start_action(sprite, STAY);
	al_sprite_set_direction(sprite, ALLEGRO_SPRITE_LEFT);
	return 0;
}

static int game_init(void)
{
	/* Init allegro engine */
	if (!al_init()) {
		fprintf(stderr, "failed to initialize allegro!\n");
		return -1;
	}

	/* Init keyboard & mouse support */
	al_install_keyboard();
	al_install_mouse();

	/* Init image addon */
	al_init_image_addon();

	/* Create a window */
	display = al_create_display(BG_WIDTH, BG_HEIGHT);
	if (!display) {
		fprintf(stderr, "failed to create display!\n");
		return -1;
	}

	/* Create event queue */
	eventq = al_create_event_queue();
	if (!eventq) {
		fprintf(stderr, "failed to create event queue!\n");
		return -1;
	}

	/* Create timer */
	timer = al_create_timer(1.0/FPS);
	if (!timer) {
		fprintf(stderr, "failed to create timer!\n");
		return -1;
	}

	if (game_init_map()) {
		fprintf(stderr, "failed to init map!\n");
		return -1;
	}

	if (game_init_sprite()) {
		fprintf(stderr, "failed to init sprite!\n");
		return -1;
	}

	/* Set window title */
	al_set_window_title(display, "SPRITE!");

	/* Register event source */
	al_register_event_source(eventq, al_get_keyboard_event_source());
	al_register_event_source(eventq, al_get_display_event_source(display));
	al_register_event_source(eventq, al_get_timer_event_source(timer));

	printf("Game Start!\n");
	return 0;
}

static void game_exit(void)
{
	if (display)
		al_destroy_display(display);
	if (timer)
		al_destroy_timer(timer);
	if (sprite)
		al_destroy_sprite(sprite);
	if (eventq)
		al_destroy_event_queue(eventq);

	printf("Game Exit!\n");
}

static bool game_check_arrow_key(int keycode)
{
	if (keycode == ALLEGRO_KEY_DOWN ||
		keycode == ALLEGRO_KEY_UP ||
		keycode == ALLEGRO_KEY_RIGHT ||
		keycode == ALLEGRO_KEY_LEFT)
		return true;
	else
		return false;
}

static bool game_check_arrow_key_down(ALLEGRO_KEYBOARD_STATE *key_state)
{
	if (al_key_down(key_state, ALLEGRO_KEY_DOWN) ||
		al_key_down(key_state, ALLEGRO_KEY_UP) ||
		al_key_down(key_state, ALLEGRO_KEY_RIGHT) ||
		al_key_down(key_state, ALLEGRO_KEY_LEFT))
		return true;
	else
		return false;
}


static void game_handle_walk_action(ALLEGRO_EVENT *event)
{
	int step_x = 0;
	int step_y = 0;
	int fps_interval;

	fps_interval = al_sprite_action_fps_interval(sprite);
	if (event->timer.count % fps_interval != 0)
		return;

	/* Update action animation counter */
	al_sprite_update_action(sprite);
	redraw = true;

	switch (al_sprite_get_direction(sprite)) {
		case ALLEGRO_SPRITE_DOWN:
			step_y = 4;
			break;
		case ALLEGRO_SPRITE_UP:
			step_y = -4;
			break;
		case ALLEGRO_SPRITE_RIGHT:
			step_x = 4;
			break;
		case ALLEGRO_SPRITE_LEFT:
			step_x = -4;
			break;
		default:
			break;
	}

	al_sprite_move_step(sprite, step_x, step_y);
}

static void game_handle_jump_action(ALLEGRO_EVENT *event)
{
	ALLEGRO_KEYBOARD_STATE key_state;
	int step_x = 0;
	int step_y = 0;
	int counter;
	int counter_max;
	int fps_interval;

	fps_interval = al_sprite_action_fps_interval(sprite);
	if (event->timer.count % fps_interval != 0)
		return;

	al_get_keyboard_state(&key_state);

	/* Update action animation counter */
	al_sprite_update_action(sprite);
	redraw = true;

	counter = al_sprite_action_counter(sprite);
	counter_max = al_sprite_action_counter_max(sprite);

	if (game_check_arrow_key_down(&key_state)){
		if (al_key_down(&key_state, ALLEGRO_KEY_LEFT))
			step_x = -4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_RIGHT))
			step_x = 4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_UP))
			step_y = -4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_DOWN))
			step_y = 4;
	}

	/* Check attack key */
	if (al_key_down(&key_state, ALLEGRO_KEY_A)) {
		al_sprite_start_action(sprite, JUMP_ATTACK);
		al_sprite_action_set_counter(sprite, counter);
	}

	if (counter <= counter_max/2)
		al_sprite_move_step(sprite, step_x, step_y-8);
	else if (counter <= counter_max)
		al_sprite_move_step(sprite, step_x, step_y+8);
	else
		al_sprite_start_action(sprite, STAY);
}

static void game_handle_jump_attack_action(ALLEGRO_EVENT *event)
{
	ALLEGRO_KEYBOARD_STATE key_state;
	int step_x = 0;
	int step_y = 0;
	int counter;
	int counter_max;
	int fps_interval;

	fps_interval = al_sprite_action_fps_interval(sprite);
	if (event->timer.count % fps_interval != 0)
		return;

	al_get_keyboard_state(&key_state);

	/* Update action animation counter */
	al_sprite_update_action(sprite);
	redraw = true;

	counter = al_sprite_action_counter(sprite);
	counter_max = al_sprite_action_counter_max(sprite);

	if (game_check_arrow_key_down(&key_state)){
		if (al_key_down(&key_state, ALLEGRO_KEY_LEFT))
			step_x = -4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_RIGHT))
			step_x = 4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_UP))
			step_y = -4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_DOWN))
			step_y = 4;
	}

	if (counter <= counter_max/2)
		al_sprite_move_step(sprite, step_x, step_y-8);
	else if (counter <= counter_max)
		al_sprite_move_step(sprite, step_x, step_y+8);
	else
		al_sprite_start_action(sprite, STAY);
}

static void game_handle_attack_action(ALLEGRO_EVENT *event)
{
	ALLEGRO_KEYBOARD_STATE key_state;
	int step_x = 0;
	int step_y = 0;
	int counter;
	int counter_max;
	int fps_interval;

	fps_interval = al_sprite_action_fps_interval(sprite);
	if (event->timer.count % fps_interval != 0)
		return;

	al_get_keyboard_state(&key_state);

	/* Update action animation counter */
	al_sprite_update_action(sprite);
	redraw = true;

	counter = al_sprite_action_counter(sprite);
	counter_max = al_sprite_action_counter_max(sprite);

	if (game_check_arrow_key_down(&key_state)){
		if (al_key_down(&key_state, ALLEGRO_KEY_LEFT))
			step_x = -4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_RIGHT))
			step_x = 4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_UP))
			step_y = -4;
		else if (al_key_down(&key_state, ALLEGRO_KEY_DOWN))
			step_y = 4;
	}

	if (counter > counter_max)
		al_sprite_start_action(sprite, STAY);
	else if (step_x != 0 || step_y != 0)
		al_sprite_move_step(sprite, step_x, step_y);
}

static void game_handle_timer_event(ALLEGRO_EVENT *event)
{
	/* No action, just return */
	if (!al_sprite_action_running(sprite))
		return;

	if (al_sprite_action_id(sprite) == WALK)
		game_handle_walk_action(event);
	else if (al_sprite_action_id(sprite) == JUMP)
		game_handle_jump_action(event);
	else if (al_sprite_action_id(sprite) == JUMP_ATTACK)
		game_handle_jump_attack_action(event);
	else if (al_sprite_action_id(sprite) == ATTACK)
		game_handle_attack_action(event);

	/* Continue walking if arrow key is down */
	if (al_sprite_action_id(sprite) == STAY) {
		ALLEGRO_KEYBOARD_STATE key_state;
		al_get_keyboard_state(&key_state);
		if (game_check_arrow_key_down(&key_state))
			al_sprite_start_action(sprite, WALK);
	}
}

static void game_handle_key_down_event(int keycode)
{
	/* Start to jump */
	if (keycode == ALLEGRO_KEY_SPACE) {
		if (al_sprite_action_stopable(sprite)) {
			al_sprite_start_action(sprite, JUMP);
			redraw = true;
		}
	}
	/* Start to attack */
	else if (keycode == ALLEGRO_KEY_A) {
		if (al_sprite_action_stopable(sprite)) {
			al_sprite_start_action(sprite, ATTACK);
			redraw = true;
		}
	}
	/* Start to walk */
	else if (game_check_arrow_key(keycode)) {
		if (al_sprite_action_stopable(sprite))
			al_sprite_start_action(sprite, WALK);

		/* Update sprite direction */
		switch (keycode) {
			case ALLEGRO_KEY_DOWN:
				al_sprite_set_direction(sprite, ALLEGRO_SPRITE_DOWN);
				break;
			case ALLEGRO_KEY_UP:
				al_sprite_set_direction(sprite, ALLEGRO_SPRITE_UP);
				break;
			case ALLEGRO_KEY_RIGHT:
				al_sprite_set_direction(sprite, ALLEGRO_SPRITE_RIGHT);
				break;
			case ALLEGRO_KEY_LEFT:
				al_sprite_set_direction(sprite, ALLEGRO_SPRITE_LEFT);
				break;
			default:
				break;
		}

		redraw = true;
	}
}

static void game_handle_key_up_event(int keycode)
{
	/* Stop walking */
	if (game_check_arrow_key(keycode)
		&& al_sprite_action_id(sprite) == WALK) {
		al_sprite_start_action(sprite, STAY);
		redraw = true;
	}
}

static void game_handle_event(ALLEGRO_EVENT *event)
{
	switch (event->type) {
		case ALLEGRO_EVENT_TIMER:
			game_handle_timer_event(event);
			break;
		case ALLEGRO_EVENT_KEY_DOWN:
			game_handle_key_down_event(event->keyboard.keycode);
			break;
		case ALLEGRO_EVENT_KEY_UP:
			game_handle_key_up_event(event->keyboard.keycode);
			break;
		case ALLEGRO_EVENT_KEY_CHAR:
			if (event->keyboard.keycode == ALLEGRO_KEY_ESCAPE)
				running = false;
			break;
		case ALLEGRO_EVENT_DISPLAY_CLOSE:
			running = false;
			break;
		default:
			break;
	}
}

static int game_draw_map(void)
{
	return 0;
}

static int game_draw_sprite(void)
{
	al_draw_sprite(sprite);
	return 0;
}

static int game_display_refresh(void)
{
	al_clear_to_color(BG_COLOR);
	game_draw_map();
	game_draw_sprite();
	al_flip_display();
	redraw = false;
	return 0;
}

static void game_loop(void)
{
	ALLEGRO_EVENT event;

	al_start_timer(timer);

	while (running) {
		if (al_is_event_queue_empty(eventq)) {
			if (redraw)
				game_display_refresh();
			continue;
		}

		al_wait_for_event(eventq, &event);
		game_handle_event(&event);
	}

	al_stop_timer(timer);
}

int main(int argc, char **argv)
{
	if (!game_init()) {
		game_loop();
	}
	game_exit();
	return 0;
}
