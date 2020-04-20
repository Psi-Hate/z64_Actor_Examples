#include <z64ovl/oot/u10.h>
#include <z64ovl/oot/helpers.h>
#include <z64ovl/oot/sfx.h>

#define ACT_ID 0x013B
#define OBJ_ID 0x0086


/* Display Lists * * * */
     #define   DL_ARROW     0x06002310



typedef struct {
    z64_actor_t actor;
    z64_actor_t dist_from_link_xz;

    z64_collider_cylinder_main_t Collision;

    vec3f_t direction;
    vec3s_t tRot;

    int count;
    int fps;
    int seconds;

} entity_t;


const uint32_t damaginghitbox[] =
{
    0x0A110009, 0x20010000, 0x00000000, 0xFFCFFFFF, 
    0x02050000, 0xFFCFFFFF, 0x00000000, 0x01010100, //Damage Amount 00 burn 01 freeze 02 shock 03 KB 04
    0x00200080, 0x00000000, 0x00000000 // Damage Radius
};

const uint32_t unkchart[] = 
{
    0x0100000F, 0x001E1E00
};
 
/*** functions ***/
static void init(entity_t *en, z64_global_t *gl)
{

    actor_set_height(&en->actor, 10);
    actor_set_scale(&en->actor, 0.02f);
    play_sound_global_once(NA_SE_IT_BOW_FLICK);
 
 	actor_collision_routine(gl, &en->actor, 50.0f, 10.0f, 100.0f, 5); //extern void external_func_8002E4B4(z64_global_t *global, z64_actor_t *actor, f32 below, f32 radius, f32 above, u32 flags);

    actor_init_shadow(&(en->actor).rot_2, 0, &ACTOR_SHADOW_DRAWFUNC_CIRCLE, 2.0f);

    actor_collider_cylinder_alloc(gl, &en->Collision);
    actor_collider_cylinder_init(gl, &en->Collision,&en->actor, &damaginghitbox);

        
    z64_actor_t Link = zh_get_player(gl)->actor;


    float x = en->actor.pos_2.x - Link.pos_2.x;
    float y = en->actor.pos_2.y - Link.pos_2.y - 40;
    float z = en->actor.pos_2.z - Link.pos_2.z;

    float len = math_sqrtf(x*x + y*y + z*z);
    en->direction.x = (x/len) * 30;
    en->direction.y = (y/len) * 30;
    en->direction.z = (z/len) * 30;
}
static void updateArrowMovement(entity_t *en, z64_global_t *gl)
{
    z64_actor_t Link = zh_get_player(gl)->actor;

    en->actor.rot_2.y = en->actor.rot_toward_link_y;
    en->actor.xz_dir = external_func_80078068(&en->actor.pos_2, &Link.pos_2);


    en->actor.pos_2.x -= en->direction.x;
    en->actor.pos_2.y -= en->direction.y;
    en->actor.pos_2.z -= en->direction.z;
    
    actor_collider_cylinder_update(&en->actor, &en->Collision);
	actor_collision_routine(gl, &en->actor, 50.0f, 10.0f, 100.0f, 5); //extern void external_func_8002E4B4(z64_global_t *global, z64_actor_t *actor, f32 below, f32 radius, f32 above, u32 flags);

    
    actor_collider_cylinder_update(&en->actor, &en->Collision);
    actor_collision_check_set_ac(gl,AADDR(gl, 0x011E60), &en->Collision);
	actor_collision_check_set_at(gl,AADDR(gl, 0x011E60), &en->Collision);
    actor_collision_check_set_ot(gl, (uint32_t*)(AADDR(gl,0x11e60)), &en->Collision);


    en->count++;

}

static void play(entity_t *en, z64_global_t *gl)
{
    updateArrowMovement(en, gl);

    if(en->count > 10 && (en->actor.bgcheck_flags & 0xB) || (en->Collision.body.flags_2 & 2)) //Check if actor is touching something
    {
        actor_kill(&en->actor);
    }
    if(en->count > 160 )
    {
        actor_kill(&en->actor);
    }
}
 
static void draw(entity_t *en, z64_global_t *gl)
{
    draw_dlist_opa(gl, DL_ARROW);
}

static void dest(entity_t *en, z64_global_t *gl)
{
}

const z64_actor_init_t init_vars = {
    .number = ACT_ID,
    .type = 0x05, //Enemy
    .room = 0x00,
    .flags = 0x00000010,
    .object = OBJ_ID,
    .padding = 0x0000,
    .instance_size = sizeof(entity_t),
    .init = init,
    .dest = dest,
    .main = play,
    .draw = draw
};