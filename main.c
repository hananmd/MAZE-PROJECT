#include "game.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Simple function to pause execution and wait for user input
static void wait_for_enter(const char *message_prompt) {
    printf("%s", message_prompt);
    fflush(stdout);
    char buffer[256];
    fgets(buffer, sizeof(buffer), stdin);
}

// Main turn logic for a single player
void play_turn(int player_id, Player players[3], Cell maze[NUM_FLOORS][FLOOR_WIDTH][FLOOR_LENGTH],
               Stair stairs[], int num_stairs, Pole poles[], int num_poles,
               Wall walls[], int num_walls, int flag_location[3]) {
    
    Player *current_player = &players[player_id];
    char player_letter = 'A' + player_id;
    
    printf("\n=== Player %c's Turn ===\n", player_letter);

    // Increment roll count at START of turn for accurate direction dice timing
    current_player->roll_count++;

    // Handle food poisoning effect first - player misses their turn
    if (current_player->bawana_effect == EFFECT_FOOD_POISONING) {
        current_player->bawana_turns_left--;
        printf("%c is still food poisoned and misses the turn.\n", player_letter);
        
        if (current_player->bawana_turns_left == 0) {
            // Clear effect BEFORE checking MP to avoid infinite loop
            current_player->bawana_effect = EFFECT_NONE;
            current_player->bawana_turns_left = 0;
            
            // If MP is depleted when recovering, send to Bawana
            if (current_player->movement_points <= 0) {
                int bawana_interior_cells[12][2] = {
                    {6,21}, {6,22}, {6,23}, {6,24},
                    {7,21}, {7,22}, {7,23}, {7,24},
                    {8,21}, {8,22}, {8,23}, {8,24}
                };
                int random_cell_idx = rand() % 12;
                current_player->pos[0] = 0;
                current_player->pos[1] = bawana_interior_cells[random_cell_idx][0];
                current_player->pos[2] = bawana_interior_cells[random_cell_idx][1];
                
                // Get the cell type for proper message display
                int cell_effect_type = maze[current_player->pos[0]][current_player->pos[1]][current_player->pos[2]].bawana_cell_type;
                const char* effect_type_names[] = {"food poisoning", "disoriented", "triggered", "happy", "random MP"};
                const char* effect_name = (cell_effect_type >= 0 && cell_effect_type < 5) ? effect_type_names[cell_effect_type] : "random";
                
                printf("%c is now fit to proceed from the food poisoning episode and now placed on a %s cell and the effects take place.\n", player_letter, effect_name);
                apply_bawana_effect(current_player, maze, player_id);
            } else {
                printf("%c has recovered from food poisoning and can resume normal play.\n", player_letter);
            }
        }
        return; // Skip rest of turn due to food poisoning
    }
    
    // Direction dice logic - each player has their own timing based on their individual roll count
    int should_roll_direction_dice = (current_player->in_game && current_player->roll_count % 4 == 0);
    const char* rolled_direction_name = "";

    if (should_roll_direction_dice) {
        int direction_roll = roll_direction_dice();
        printf("Rolling direction die: %d\n", direction_roll);
        
        // If at Bawana entrance, force direction to North and ignore the die
        if (maze[current_player->pos[0]][current_player->pos[1]][current_player->pos[2]].is_bawana_entrance) {
            printf("Direction die: %d (ignored at Bawana entrance)\n", direction_roll);
            current_player->direction = DIR_NORTH;
            rolled_direction_name = "North";
            printf("Direction forced to: %s (Bawana entrance)\n", get_direction_name(current_player->direction));
        } else {
            // Map die roll to direction
            if (direction_roll == 2) { 
                current_player->direction = DIR_NORTH; 
                rolled_direction_name = "North";
            } else if (direction_roll == 3) { 
                current_player->direction = DIR_EAST;  
                rolled_direction_name = "East";
            } else if (direction_roll == 4) { 
                current_player->direction = DIR_SOUTH; 
                rolled_direction_name = "South";
            } else if (direction_roll == 5) { 
                current_player->direction = DIR_WEST;  
                rolled_direction_name = "West";
            } else { 
                rolled_direction_name = "Empty"; // Roll of 1 or 6 means no change
            }
            
            printf("Direction die: %d (%s)\n", direction_roll, rolled_direction_name);
            
            if (direction_roll == 1 || direction_roll == 6) {
                printf("Direction unchanged: %s\n", get_direction_name(current_player->direction));
            } else {
                printf("Direction changed to: %s\n", get_direction_name(current_player->direction));
            }
        }
    }
    
    // Roll movement die
    int movement_roll = roll_movement_dice();
    printf("Rolling movement die: %d\n", movement_roll);
    
    // Handle players in starting area (need to roll 6 to enter maze)
    if (!current_player->in_game) {
        if (movement_roll == 6) {
            // Check if player is at Player A's starting area (after being captured/reset)
            if (current_player->pos[0] == 0 && current_player->pos[1] == 6 && current_player->pos[2] == 12) {
                // Enter maze like Player A
                enter_maze_like_player_a(current_player);
                printf("%c is at Player A's starting area and rolls 6 on the movement dice and is placed on Player A's first maze cell %s.\n", 
                       player_letter, format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
            } else {
                // Normal entry for players at their original starting positions
                enter_maze(current_player, player_id);
                printf("%c is at the starting area and rolls 6 on the movement dice and is placed on %s of the maze.\n", 
                       player_letter, format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
            }
            
            printf("%c moved 0 cells that cost 0 movement points and is left with %d and is moving in the %s.\n", 
                   player_letter, current_player->movement_points, get_direction_name(current_player->direction));
            
            return;
        } else {
            printf("%c is at the starting area and rolls %d on the movement dice and cannot enter the maze.\n", player_letter, movement_roll);
            
            // If MP is depleted, send to Bawana for replenishment
            if (current_player->movement_points <= 0) {
                reset_to_bawana(current_player, player_id);
                // Apply Bawana effects after transportation
                apply_bawana_effect(current_player, maze, player_id);
            }
            
            return;
        }
    }

    // Player is in the maze - handle actual movement
    int original_dice_roll = movement_roll;
    int movement_cost_total = 0;
    int steps_actually_taken = 0;
    int movement_blocked_reason = BLOCK_NONE;
    
    // Apply Bawana effects that modify movement
    if (current_player->bawana_effect == EFFECT_DISORIENTED) {
        current_player->direction = rand() % 4; // Random direction when disoriented
    } else if (current_player->bawana_effect == EFFECT_TRIGGERED) {
        movement_roll *= 2; // Triggered players move double the rolled amount
    }

    // Store position before movement for comparison
    int position_before_move[3] = {current_player->pos[0], current_player->pos[1], current_player->pos[2]};
    
    // Print appropriate movement message based on current state
    if (should_roll_direction_dice) {
        if (current_player->bawana_effect == EFFECT_TRIGGERED) {
            printf("%c is triggered and rolls and %d on the movement dice and move in the %s and moves %d cells", 
                   player_letter, original_dice_roll, get_direction_name(current_player->direction), movement_roll);
        } else if (current_player->bawana_effect == EFFECT_DISORIENTED) {
            printf("%c rolls and %d on the movement dice and is disoriented and move in the %s and moves %d cells", 
                   player_letter, original_dice_roll, get_direction_name(current_player->direction), original_dice_roll);
        } else {
            printf("%c rolls and %d on the movement dice and %s on the direction dice, changes direction to %s and moves %d cells", 
                   player_letter, original_dice_roll, rolled_direction_name, get_direction_name(current_player->direction), original_dice_roll);
        }
    } else {
        if (current_player->bawana_effect == EFFECT_TRIGGERED) {
            printf("%c is triggered and rolls and %d on the movement dice and move in the %s and moves %d cells", 
                   player_letter, original_dice_roll, get_direction_name(current_player->direction), movement_roll);
        } else if (current_player->bawana_effect == EFFECT_DISORIENTED) {
            printf("%c rolls and %d on the movement dice and is disoriented and move in the %s and moves %d cells", 
                   player_letter, original_dice_roll, get_direction_name(current_player->direction), original_dice_roll);
        } else {
            printf("%c rolls and %d on the movement dice and moves %s by %d cells", 
                   player_letter, original_dice_roll, get_direction_name(current_player->direction), original_dice_roll);
        }
    }
    
    // Attempt movement with comprehensive error handling
    (void)move_player_with_teleport(current_player, players, maze, stairs, num_stairs, 
                                  poles, num_poles, walls, num_walls, 
                                  movement_roll, player_id, flag_location,
                                  &movement_cost_total, &steps_actually_taken, &movement_blocked_reason);

    // Generate appropriate output based on movement results
    if (position_before_move[0] == current_player->pos[0] && position_before_move[1] == current_player->pos[1] && position_before_move[2] == current_player->pos[2]) {
        // Player didn't move (blocked by something)
        if (movement_blocked_reason != BLOCK_NONE) {
            printf(" and cannot move in the %s due to %s. Player remains at %s\n", 
                   get_direction_name(current_player->direction), 
                   get_blockage_reason_description(movement_blocked_reason),
                   format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
        } else {
            printf(" and cannot move in the %s. Player remains at %s\n", 
                   get_direction_name(current_player->direction), 
                   format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
        }
        
        // Deduct movement cost even when blocked
        current_player->movement_points -= movement_cost_total;
        
        printf("%c moved 0 cells that cost %d movement points and is left with %d and is moving in the %s.\n", 
               player_letter, movement_cost_total, current_player->movement_points, get_direction_name(current_player->direction));
    } else {
        // Player successfully moved
        current_player->movement_points -= movement_cost_total;
        
        // Different message formats based on Bawana effects
        if (current_player->bawana_effect == EFFECT_DISORIENTED) {
            printf(" and moves %d cells and is placed at %s.\n", steps_actually_taken, format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
        } else if (current_player->bawana_effect == EFFECT_TRIGGERED) {
            printf(" and moves %d cells and is placed at %s.\n", steps_actually_taken, format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
        } else {
            printf(" and is now at %s.\n", format_position(current_player->pos[0], current_player->pos[1], current_player->pos[2]));
        }
        
        printf("%c moved %d cells that cost %d movement points and is left with %d and is moving in the %s.\n", 
               player_letter, steps_actually_taken, movement_cost_total, current_player->movement_points, get_direction_name(current_player->direction));
    }

    // Handle countdown for other Bawana effects
    if (current_player->bawana_effect > EFFECT_NONE && current_player->bawana_effect != EFFECT_FOOD_POISONING && current_player->bawana_effect != EFFECT_HAPPY) {
        current_player->bawana_turns_left--;
        if (current_player->bawana_turns_left == 0) {
            if (current_player->bawana_effect == EFFECT_DISORIENTED) {
                printf("%c has recovered from disorientation.\n", player_letter);
            } else if (current_player->bawana_effect == EFFECT_TRIGGERED) {
                printf("%c has recovered from being triggered.\n", player_letter);
            } else if (current_player->bawana_effect == EFFECT_RANDOM_MP) {
                printf("%c's random movement point effect has expired.\n", player_letter);
            }
            current_player->bawana_effect = EFFECT_NONE;
        }
    }

    // Check if movement points are completely depleted
    if (current_player->movement_points <= 0) {
        reset_to_bawana(current_player, player_id);
        // Apply Bawana effects after transportation
        apply_bawana_effect(current_player, maze, player_id);
    }

    // Check for player captures (when players occupy the same cell)
    check_player_capture(players, player_id);

    // Check if player captured the flag (win condition)
    if (check_flag_capture(current_player, flag_location)) {
        printf("Player %c has captured the flag!\n", player_letter);
        printf("Player %c wins the game!\n", player_letter);
        exit(0); // Game over
    }
}

// Display current game state for all players
void print_game_status(Player players[3], int flag_location[3]) {
    printf("\n--- Game Status ---\n");
    printf("Flag location: [%d,%d,%d]\n", flag_location[0], flag_location[1], flag_location[2]);
    
    for (int player_idx = 0; player_idx < 3; player_idx++) {
        char player_letter = 'A' + player_idx;
        printf("Player %c: [%d,%d,%d] - ", player_letter, players[player_idx].pos[0], players[player_idx].pos[1], players[player_idx].pos[2]);
        
        if (players[player_idx].in_game) {
            printf("In maze, MP: %d", players[player_idx].movement_points);
            if (players[player_idx].bawana_effect > EFFECT_NONE) {
                const char* bawana_effect_names[] = {"None", "Food Poisoning", "Disoriented", "Triggered", "Happy", "Random MP"};
                printf(", Bawana effect: %s", bawana_effect_names[players[player_idx].bawana_effect]);
                if (players[player_idx].bawana_effect != EFFECT_HAPPY) {
                    printf(" (turns left: %d)", players[player_idx].bawana_turns_left);
                }
            }
        } else {
            printf("In starting area, MP: %d", players[player_idx].movement_points);
        }
        printf("\n");
    }
    printf("-------------------\n");
}

// Main game loop and initialization
int main(void) {
    // Initialize log file for assumptions, warnings, and loop detection
    init_log_file();
    
    // Try to load seed from file, otherwise use current time
    int random_seed = read_seed_from_file("seed.txt");
    srand((unsigned int)random_seed);
    printf("Using seed: %d\n", random_seed);
    fflush(stdout);
    
    // Initialize all game data structures
    Cell maze_structure[NUM_FLOORS][FLOOR_WIDTH][FLOOR_LENGTH];
    Player game_players[3];
    Stair stair_connections[MAX_STAIRS];
    Pole pole_slides[MAX_POLES];
    Wall maze_walls[MAX_WALLS];
    int total_stairs, total_poles, total_walls;
    int flag_position[3];
    
    // Set up basic game structure
    printf("Initializing maze structure...\n");
    fflush(stdout);
    initialize_maze(maze_structure);

    printf("Initializing players...\n");
    fflush(stdout);
    initialize_players(game_players);
    
    // Try to load stairs from file, use defaults if file not found
    if (!read_stairs_from_file("stairs.txt", stair_connections, &total_stairs)) {
        initialize_stairs(stair_connections, &total_stairs);
        printf("Using default stairs configuration.\n");
        fflush(stdout);
    } else {
        printf("Loaded stairs from stairs.txt\n");
        fflush(stdout);
    }
    
    // Try to load poles from file, use defaults if file not found
    if (!read_poles_from_file("poles.txt", pole_slides, &total_poles)) {
        initialize_poles(pole_slides, &total_poles);
        printf("Using default poles configuration.\n");
        fflush(stdout);
    } else {
        printf("Loaded poles from poles.txt\n");
        fflush(stdout);
    }
    
    // Try to load walls from file, use defaults if file not found
    if (!read_walls_from_file("walls.txt", maze_walls, &total_walls)) {
        initialize_walls(maze_walls, &total_walls);
        printf("Using default walls configuration.\n");
        fflush(stdout);
    } else {
        printf("Loaded walls from walls.txt\n");
        fflush(stdout);
    }

    // Sanitize walls to disable segments that overlap critical cells
    sanitize_walls(maze_walls, &total_walls);
    
    // Handle stair blocking for multi-floor stairs (prevent skipping floors)
    for (int stair_idx = 0; stair_idx < total_stairs; stair_idx++) {
        int start_floor = stair_connections[stair_idx].start_floor;
        int end_floor = stair_connections[stair_idx].end_floor;
        int start_width = stair_connections[stair_idx].start_w;
        int start_length = stair_connections[stair_idx].start_l;
        int end_width = stair_connections[stair_idx].end_w;
        int end_length = stair_connections[stair_idx].end_l;
        
        int min_floor = (start_floor < end_floor) ? start_floor : end_floor;
        int max_floor = (start_floor > end_floor) ? start_floor : end_floor;
        
        // If stair spans more than 1 floor, block intermediate floors
        if (max_floor - min_floor > 1) {
            // Block intermediate floors at both start and end coordinates (for angled stairs)
            for (int blocked_floor = min_floor + 1; blocked_floor < max_floor; blocked_floor++) {
                // Block at start coordinates
                if (start_width >= 0 && start_width < FLOOR_WIDTH &&
                    start_length >= 0 && start_length < FLOOR_LENGTH) {
                    maze_structure[blocked_floor][start_width][start_length].is_blocked_by_stair = 1;
                    printf("Blocked cell [%d,%d,%d] for skipping stair (start coordinates).\n", blocked_floor, start_width, start_length);

                    char log_msg[200];
                    snprintf(log_msg, sizeof(log_msg), "ASSUMPTION: Blocked cell [%d,%d,%d] for stair floor skipping prevention (start coordinates)", blocked_floor, start_width, start_length);
                    write_to_log(log_msg);
                }

                // Block at end coordinates if different from start (angled stairs)
                if ((end_width != start_width || end_length != start_length) &&
                    end_width >= 0 && end_width < FLOOR_WIDTH &&
                    end_length >= 0 && end_length < FLOOR_LENGTH) {
                    maze_structure[blocked_floor][end_width][end_length].is_blocked_by_stair = 1;
                    printf("Blocked cell [%d,%d,%d] for skipping stair (end coordinates).\n", blocked_floor, end_width, end_length);

                    char log_msg[200];
                    snprintf(log_msg, sizeof(log_msg), "ASSUMPTION: Blocked cell [%d,%d,%d] for stair floor skipping prevention (end coordinates)", blocked_floor, end_width, end_length);
                    write_to_log(log_msg);
                }
            }
        }
    }
    
    // Load or randomly place the flag, then validate and ensure reachability
    int loaded_from_file = read_flag_from_file("flag.txt", flag_position);
    if (!loaded_from_file) {
        place_random_flag(flag_position, maze_structure);
        printf("Flag randomly placed at [%d,%d,%d]\n", flag_position[0], flag_position[1], flag_position[2]);
    } else {
        if (!is_valid_flag_cell(maze_structure, flag_position[0], flag_position[1], flag_position[2])) {
            printf("Flag in flag.txt at %s is invalid. Replacing with a random valid location.\n", 
                   format_position(flag_position[0], flag_position[1], flag_position[2]));
            char log_msg[300];
            snprintf(log_msg, sizeof(log_msg), "WARNING: Flag in flag.txt at [%d,%d,%d] is invalid, replacing with random valid location", flag_position[0], flag_position[1], flag_position[2]);
            write_to_log(log_msg);
            place_random_flag(flag_position, maze_structure);
        } else if (!is_flag_reachable(maze_structure, stair_connections, total_stairs, pole_slides, total_poles, maze_walls, total_walls, flag_position)) {
            printf("Flag in flag.txt at %s is unreachable. Replacing with a random valid reachable location.\n", 
                   format_position(flag_position[0], flag_position[1], flag_position[2]));
            char log_msg[300];
            snprintf(log_msg, sizeof(log_msg), "WARNING: Flag in flag.txt at [%d,%d,%d] is unreachable, replacing with random reachable location", flag_position[0], flag_position[1], flag_position[2]);
            write_to_log(log_msg);
            place_random_flag(flag_position, maze_structure);
        }
    }

    // --- FLAG VALIDATION SECTION ---
    printf("\nFLAG VALIDATION:\n");
    printf("---------------\n");

    if (is_flag_reachable(maze_structure, stair_connections, total_stairs, pole_slides, total_poles, maze_walls, total_walls, flag_position)) {
        printf("Flag position [%d,%d,%d] is valid and reachable\n", flag_position[0], flag_position[1], flag_position[2]);
        char log_msg[200];
        snprintf(log_msg, sizeof(log_msg), "ASSUMPTION: Flag accessibility confirmed via BFS pathfinding at [%d,%d,%d]", flag_position[0], flag_position[1], flag_position[2]);
        write_to_log(log_msg);
    } else {
        printf("Flag position [%d,%d,%d] is NOT reachable!\n", flag_position[0], flag_position[1], flag_position[2]);
        printf("Game may be unwinnable. Please adjust flag position or maze layout.\n");
        char log_msg[200];
        snprintf(log_msg, sizeof(log_msg), "WARNING: Flag position [%d,%d,%d] is NOT reachable - game may be unwinnable!", flag_position[0], flag_position[1], flag_position[2]);
        write_to_log(log_msg);
    }
    // --- END OF FLAG VALIDATION ---

    printf("\n=== INITIALIZATION COMPLETE ===\n");
    fflush(stdout);

    // Display game start information
    printf("\n=== Maze of UCSC ===\n");
    fflush(stdout);
    printf("Flag is placed at [%d,%d,%d]\n", flag_position[0], flag_position[1], flag_position[2]);
    fflush(stdout);
    printf("\n=== AUTOMATIC MODE ===\n");
    fflush(stdout);
    printf("Game running automatically...\n");
    fflush(stdout);
    printf("========================\n");
    fflush(stdout);
    print_game_status(game_players, flag_position);

    printf("\nStarting the game...\n");
    fflush(stdout);

    // Main game loop - continues until someone wins
    int current_round = 1;
    printf("\n=== Starting Game Loop ===\n");
    fflush(stdout);

    while (1) {
        printf("\n=== Round %d ===\n", current_round);
        fflush(stdout);
        update_stair_directions(stair_connections, total_stairs);
        
        // Each player takes their turn in order
        for (int player_turn = 0; player_turn < 3; player_turn++) {
            play_turn(player_turn, game_players, maze_structure, stair_connections, total_stairs, 
                      pole_slides, total_poles, maze_walls, total_walls, flag_position);
        }

        print_game_status(game_players, flag_position);

        printf("\nContinuing to round %d...\n", current_round + 1);
        fflush(stdout);

        current_round++;
    }
    
    // Cleanup complete
    close_log_file();
    return 0; 
}