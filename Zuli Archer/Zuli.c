// Zuli: The Freezing Archer
// Created by: Psi-Hate
// Code Contributors: Skawo, SpiceyWolf
// Technical help: Nokaubure, z64me, CrookedPoe


#include <z64ovl/oot/u10.h>
#include <z64ovl/oot/helpers.h>
#include <z64ovl/oot/sfx.h>

#define ACT_ID 0x0082
#define OBJ_ID 0x0086

/* Display Lists * * * */
     #define   DL_ARROW     0x06002310

/* Collisions * * * */

/* Hierarchical Display Lists * * * */
     #define   LIMB_LIMB_1     0x06002930
     #define   LIMB_LIMB_2     0x06003C78
     #define   LIMB_LIMB_4     0x060044A8
     #define   LIMB_LIMB_5     0x060046B8
     #define   LIMB_LIMB_6     0x06004968
     #define   LIMB_LIMB_8     0x06004C98
     #define   LIMB_LIMB_9     0x06004F68
     #define   LIMB_LIMB_10     0x06005FA8
     #define   LIMB_LIMB_11     0x06006718
     #define   LIMB_LIMB_12     0x06006F58
     #define   LIMB_LIMB_13     0x060074E8
     #define   LIMB_LIMB_14     0x06007940
     #define   LIMB_LIMB_15     0x060080C0
     #define   LIMB_LIMB_17     0x060089B0

/* Animations * * * */
     #define   ANIM_IDLE     0x06008D00
     #define   ANIM_JUMP     0x06008EC8
     #define   ANIM_SHOOT     0x06009270

/* Hierarchies (Skeletons) * * * */
     #define   SKL_DEFAULT     0x060093A0

/* Base Offset: 0x06000000 */



typedef struct {
    z64_actor_t actor;
    z64_actor_t dist_from_link_xz;

    z64_collider_cylinder_main_t cylinderreceptor;
    z64_collider_cylinder_main_t cylinderdamage;

    z64_debug_text_t text_struct;

    z64_skelanime_t skelanime;

    uint32_t path_id;
    uint32_t shootTimer;
    uint32_t* pathlist;

    int16_t state;

    uint8_t opacity;
    uint8_t num_nodes;
    uint8_t prev_health;
    uint8_t health;

    vec3f_t next_dest;
    vec3f_t curPos;
    vec3f_t destPos;

    float last_diff;
    float jumpTime;

    bool touching_ground;
    bool isJumping;
    bool HandleComplete;

    

    int count;
    int fps;
    int seconds;

    int fightCount;

    int current_node;
    int delay;
    int invincibility;

    int anim;

} entity_t;
 
const z64_collider_cylinder_init_t receptivehitbox =
{
    .body = {
        .unk_0x14 = 0x07, 
        .collider_flags = 0x40, 
        .collide_flags = 0x09, 
        .mask_a = 0x39, 
        .mask_b = 0x10, 
        .type = 0x01, 
        .body_flags = 0x00, 
        .toucher_mask = 0x00000000, 
        .bumper_effect = 0x00, 
        .toucher_damage = 0x04, 
        .bumper_mask = 0xFFCFFFFF, 
        .toucher_flags = 0x00, 
        .bumper_flags = 0x05, 
        .body_flags_2 = 0x01
        },
    .radius = 0x0030,
    .height = 0x0050,
    .y_shift = 0,
    .position = {.x = 0, .y = 0, .z = 0}
};

const uint32_t damaginghitbox[] =
{
    0x0A110009, 0x20010000, 0x00000000, 0xFFCFFFFF, 
    0x02050000, 0xFFCFFFFF, 0x00000000, 0x01010100, //Damage Amount 00 burn 01 freeze 02 shock 03 KB 04
    0x00200080, 0x00000000, 0x00000000 // Damage Radius
};

const uint8_t damagechart[] =
{
    0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 
    0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 
    0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 
    0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2, 0xF2
};

const uint32_t unkchart[] = 
{
    0x0100000F, 0x001E1E00
};

static uint32_t* get_current_header_addr(z64_global_t *global)
{
    z64_file_t* file = zh_get_save_context(global);
    uint32_t scene_setup_index = file->scene_setup_index;
    
    uint32_t* header_addr = global->scene_file;
    
    if (*header_addr == 0x18000000 && scene_setup_index != 0)
    {
        uint32_t* headers_list = (uint32_t*)*(header_addr + 1);
        
        for (int i = scene_setup_index - 1; i > -1; i--)
        {
            uint32_t* addr = (uint32_t*)zh_seg2ram((uint32_t)(headers_list + i));
            
            if (*addr != 0)
            {    
                header_addr = (uint32_t*)zh_seg2ram((uint32_t)*addr);
                break;
            }
        }
    }

    return header_addr;
}

static uint32_t* get_path_list_addr(z64_global_t *global)
{
    uint32_t* header_addr = get_current_header_addr(global);
    
    for (int i = 0; i < 14; i++)
    {
        if (*(header_addr + (i * 2)) == 0x0D000000)
        {
            return (uint32_t*)zh_seg2ram((uint32_t)*(header_addr + (i * 2) + 1));
        } 
    }
    
    return 0;
}

static uint32_t* get_path_address(uint32_t* path_list_addr, s16 path_id)
{
    return (uint32_t*)(path_list_addr + (path_id * 2));
}

static uint8_t get_number_of_nodes_from_path(uint32_t* path_addr)
{
    return (uint8_t)*((uint8_t*)path_addr);
}

static z64_xyz_t* get_node_data_from_path(uint32_t* path_list_addr, s16 path_id, s16 node_id)
{
    uint32_t* path_addr = get_path_address(path_list_addr, path_id);
    uint8_t num_nodes = get_number_of_nodes_from_path(path_addr);
    
    if (num_nodes < node_id)
        return 0;
    else
        return (z64_xyz_t*)((zh_seg2ram((uint32_t)*(path_addr + 1))) + (node_id * 6));
}

void get_next_dest(entity_t *en, z64_global_t *gl)
{
    //sets the node data within a path to dest_pos
    vec3s_t *dest_pos = (vec3s_t*)get_node_data_from_path(en->pathlist , en->path_id, en->current_node);
    //converts vec3s data type to vec3f
    math_vec3f_from_vec3s(&en->next_dest, dest_pos);
} 
 
/*** functions ***/
static void init(entity_t *en, z64_global_t *gl)
{
    //adds  gravity
	en->actor.pos_2.y -=10;
    en->isJumping = false;


    actor_set_scale(&en->actor, 0.02f);
    actor_set_height(&en->actor, 50);
    
    en->fps = 20;
    en->count = 0;
    en->seconds = 4;

    en->actor.xz_speed = 70.0f;


	actor_collision_routine(gl, &en->actor, 50.0f, 10.0f, 100.0f, 5); //extern void external_func_8002E4B4(z64_global_t *global, z64_actor_t *actor, f32 below, f32 radius, f32 above, u32 flags);

    actor_init_shadow(&(en->actor).rot_2, 0, &ACTOR_SHADOW_DRAWFUNC_CIRCLE, 10.0f);
 
    actor_collider_cylinder_alloc(gl,&en->cylinderreceptor);
    actor_collider_cylinder_init(gl,&en->cylinderreceptor,&en->actor,&receptivehitbox);
    actor_collider_cylinder_alloc(gl,&en->cylinderdamage);
    actor_collider_cylinder_init(gl,&en->cylinderdamage,&en->actor,&damaginghitbox);

    external_func_80061ED4(AADDR(&en->actor,0x98),&damagechart,&unkchart); // damage chart

    actor_collider_cylinder_update(&en->actor,&en->cylinderreceptor);

    en->actor.gravity = -0.5f;

    // gets the list of paths and set first destination
    en->last_diff = 999999.0f;
    en->current_node = 0;
    en->path_id = 0;
    en->pathlist = get_path_list_addr(gl);
    en->num_nodes = get_number_of_nodes_from_path(get_path_address(en->pathlist, en->path_id));
    get_next_dest(en, gl);
    en->actor.mass = 0xF0;

    skelanime_init_mtx(gl, &en->skelanime, SKL_DEFAULT, ANIM_IDLE, 0, 0, 0);

    en->actor.health = 30;
    en->prev_health = 30;
}

static void HandleShoot(entity_t *en, z64_global_t *gl)
{
    //Set Animation to shooting arrow
    en->anim = ANIM_SHOOT; //en->anim = ANIM_SHOOT;
    actor_anime_change(
        &en->skelanime, en->anim, 1.0,
        0, 0, 0, 0
    );

    en->actor.rot_2.y = en->actor.rot_toward_link_y; //Rotates towards Link
    
    if(en->count == 80) // If 5 seconds have passed since touching the ground, attack
    {
        play_sound_global_once(NA_SE_IT_BOW_DRAW);
        actor_spawn(AADDR(gl,0x1C24), gl, 0x013B, en->actor.pos_2.x, en->actor.pos_2.y + 13, en->actor.pos_2.z, 0, 0, 0, 0);
        
    }
}

void HandleJump(entity_t *en, z64_global_t *gl) 
{
    
    if (en->isJumping) 
    {
        //Set Animation to shooting arrow
        en->anim = ANIM_JUMP; //en->anim = ANIM_SHOOT;
        actor_anime_change(
            &en->skelanime, en->anim, 1.0,
            0, 0, 0, 0
        );

        en->jumpTime += (1.0f / 20.0f);
        float delta = en->jumpTime;

        if (delta < 1.0f)
        {
            // Parabola code!
            float time = delta;
            float ptime = time * 2 - 1;

            // Multiply for temp vectors
            vec3f_t tdest = en->destPos;
            tdest.x = (tdest.x - en->curPos.x) * time;
            tdest.y = (tdest.y - en->curPos.y) * time;
            tdest.z = (tdest.z - en->curPos.z) * time;
            vec3f_t tcur = en->curPos;
            tcur.x += tdest.x;
            tcur.y += tdest.y;
            tcur.z += tdest.z;

            tcur.y += (-ptime * ptime + 1) * 100;
            en->actor.pos_2 = tcur;
        }
        else if (delta >= 1.0f)
        {
            // Reset timer
            en->jumpTime = 0.0f;

            // Prevent desync.
            en->actor.pos_2 = en->destPos;

            en->isJumping = false;
        }
    }
    else 
    {
        if((en->actor.bgcheck_flags & 0xB) ) //Check if actor is touching something
        {
            //Set Animation to shooting arrow
            en->anim = ANIM_IDLE; //en->anim = ANIM_SHOOT;
            actor_anime_change(
            &en->skelanime, en->anim, 1.0,
            0, 0, 0, 0
            );  
                
            if (en->count > 120) // every 7 seconds, jump again
            {
                en->isJumping = true;
                en->count = 0;
            }

            else if(en->count > 40) // After 2 seconds have passed while on the ground
            {
                HandleShoot(en, gl); // Start the attack process
                
            }

            else // While the first 2 seconds are going on after touching the ground
            {
                en->actor.xz_speed = 100;
                get_next_dest(en, gl); 
                
                en->curPos = en->actor.pos_2;
                en->destPos = en->next_dest;
                en->actor.rot_2.y = external_func_80078068(&en->actor.pos_2, &en->next_dest);

            }
            en->count++; //Should add +1 every frame that the actor is touching the ground
        }
        else //Move towards the current direction just in case
        {
            en->actor.xz_speed = 0;
            en->actor.gravity = -5;
            en->actor.pos_2.y = 5;
            actor_move_towards_direction(&en->actor);
        }
    }
}


static void play(entity_t *en, z64_global_t *gl)
{

    HandleJump(en, gl);    

    
    


    float diff = ABS(math_vec3f_distance(&en->next_dest, &en->actor.pos_2));
        

    if(diff < en->actor.xz_speed)
    {
        en->current_node += 1;
    }
    if (en->current_node == en->num_nodes - 1)
    {
        en->current_node = 0;
    }

    actor_update_pos(&en->actor);


    actor_collision_check_set_ot(gl,AADDR(gl,0x11E60),&en->cylinderreceptor);
    actor_collision_check_set_ac(gl,AADDR(gl,0x11E60),&en->cylinderreceptor);

    actor_collider_cylinder_update(&en->actor,&en->cylinderdamage);
    actor_collision_check_set_at(gl,AADDR(gl,0x11E60),&en->cylinderdamage);
    actor_collision_check_set_ot(gl,AADDR(gl,0x11E60),&en->cylinderdamage);

    
    if ( en->actor.health != en->prev_health ) // If Zuli is hurt
    {

        play_sound_global_once(NA_SE_EN_TWINROBA_YOUNG_DAMAGE);
        en->actor.damage_color = 0x5FF1;
        en->actor.health -= 1;

        if (en->actor.health <= 0 || en->actor.health > 250)
        {
            sound_set_bgm(0x21);
            actor_spawn(AADDR(gl,0x1C24), gl, 0x005D, 0, 0, 0, 15.0f, 15.0f, 15.0f, 0x0000);
            actor_kill(&en->actor);
        }
    }
    en->prev_health = en->actor.health;
    actor_update_health(&en->actor);

    zh_draw_debug_text(gl, 0xFFFFFFFF, 1, 1, "health: %d", en->actor.health);
    actor_collider_cylinder_update(&en->actor,&en->cylinderreceptor);
}
 
static void draw(entity_t *en, z64_global_t *gl)
{
    // update the transformation matrices for each limb, based on the current animation frame
	actor_anime_frame_update_mtx(&en->skelanime);
	
	// draw a matrix-enabled skeleton
	skelanime_draw_mtx(
		gl,
		en->skelanime.limb_index,
		en->skelanime.unk5,
		en->skelanime.dlist_count,
		0, 0,
		&en->actor
	);


}

static void dest(entity_t *en, z64_global_t *gl)
{
}

const z64_actor_init_t init_vars = {
    .number = ACT_ID,
    .type = 0x05, //Enemy
    .room = 0x00,
    .flags = 0x00000011,
    .object = OBJ_ID,
    .padding = 0x0000,
    .instance_size = sizeof(entity_t),
    .init = init,
    .dest = dest,
    .main = play,
    .draw = draw
};