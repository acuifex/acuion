#include "autostrafe.h"

#include "../settings.h"
#include "../interfaces.h"
#include "../Utils/math.h"
#include <cmath>

static void LegitStrafe(C_BasePlayer* localplayer, CUserCmd* cmd)
{
	if (localplayer->GetFlags() & FL_ONGROUND)
		return;

	if (cmd->buttons & IN_FORWARD || cmd->buttons & IN_BACK || cmd->buttons & IN_MOVELEFT || cmd->buttons & IN_MOVERIGHT)
		return;

	if (cmd->mousedx <= 1 && cmd->mousedx >= -1)
		return;

	switch (Settings::AutoStrafe::type)
	{
		case AutostrafeType::AS_FORWARDS:
			cmd->sidemove = cmd->mousedx < 0.f ? -250.f : 250.f;
			break;
		case AutostrafeType::AS_BACKWARDS:
			cmd->sidemove = cmd->mousedx < 0.f ? 250.f : -250.f;
			break;
		case AutostrafeType::AS_LEFTSIDEWAYS:
			cmd->forwardmove = cmd->mousedx < 0.f ? -250.f : 250.f;
			break;
		case AutostrafeType::AS_RIGHTSIDEWAYS:
			cmd->forwardmove = cmd->mousedx < 0.f ? 250.f : -250.f;
			break;
		default:
			break;
	}
}

static void RageStrafe(C_BasePlayer* localplayer, CUserCmd* cmd)
{
	static bool leftRight;
	bool inMove = cmd->buttons & IN_FORWARD || cmd->buttons & IN_BACK || cmd->buttons & IN_MOVELEFT || cmd->buttons & IN_MOVERIGHT;

	if (cmd->buttons & IN_FORWARD && localplayer->GetVelocity().Length() <= 50.0f)
		cmd->forwardmove = 250.0f;

	float yaw_change = 0.0f;
	if (localplayer->GetVelocity().Length() > 50.f)
		yaw_change = 30.0f * fabsf(30.0f / localplayer->GetVelocity().Length());

	C_BaseCombatWeapon* activeWeapon = (C_BaseCombatWeapon*) entityList->GetClientEntityFromHandle(localplayer->GetActiveWeapon());
	if (activeWeapon && !activeWeapon->GetAmmo() == 0 && cmd->buttons & IN_ATTACK)
		yaw_change = 0.0f;

	QAngle viewAngles;
	engine->GetViewAngles(viewAngles);

	if (!(localplayer->GetFlags() & FL_ONGROUND) && !inMove)
	{
		if (leftRight || cmd->mousedx > 1)
		{
			viewAngles.y += yaw_change;
			cmd->sidemove = 250.0f;
		}
		else if (!leftRight || cmd->mousedx < 1)
		{
			viewAngles.y -= yaw_change;
			cmd->sidemove = -250.0f;
		}

		leftRight = !leftRight;
	}

	Math::NormalizeAngles(viewAngles);
	Math::ClampAngles(viewAngles);

	Math::CorrectMovement(viewAngles, cmd, cmd->forwardmove, cmd->sidemove);

	if (!Settings::AutoStrafe::silent)
		cmd->viewangles = viewAngles;
	
	
	// copypasta https://www.unknowncheats.me/forum/counterstrike-global-offensive/356497-multidirectional-autostrafer.html
	// Define rough directions
	/*enum directions {
		FORWARDS = 0,
		BACKWARDS = 180,
		LEFT = 90,
		RIGHT = -90
	};
	
	// Reset direction when player is not strafing
	// bool is_bhopping = cmd->buttons & IN_JUMP;
	static float calculated_direction = 0;
	static bool in_transition = false;
	if ( !(cmd->buttons & IN_JUMP) && localplayer->GetFlags() & FL_ONGROUND ) {
		calculated_direction = directions::FORWARDS;
		in_transition = false;
		return;
	}
	
	// Get true view angles
	QAngle base;
	engine->GetViewAngles(base);
	
	// Calculate the rough direction closest to the player's true direction
	auto get_rough_direction = [ & ]( float true_direction ) -> float {
		// Make array with our four rough directions
		std::array< float, 4 > minimum = { directions::FORWARDS, directions::BACKWARDS, directions::LEFT, directions::RIGHT };
		float best_angle, best_delta = 181.f;
		
		// Loop through our rough directions and find which one is closest to our true direction
		for ( size_t i = 0; i < minimum.size( ); ++i ) {
			float rough_direction = base.y + minimum.at( i );
			float delta = fabsf( Math::ReturningNormalizeYaw( true_direction - rough_direction ) );
			
			// Only the smallest delta wins out
			if ( delta < best_delta ) {
				best_angle = rough_direction;
				best_delta = delta;
			}
		}
		
		return best_angle;
	};
	
	// Get true direction based on player velocity
	Vector vel = localplayer->GetVelocity();
	QAngle angles;
	Math::VectorAngles(vel, angles);
	float true_direction = angles.y;
	float wish_direction;
	
	// Detect wish direction based on movement keypresses
	if ( cmd->buttons & IN_FORWARD ) {
		wish_direction = base.y + directions::FORWARDS;
	}
	else if ( cmd->buttons & IN_BACK ) {
		wish_direction = base.y + directions::BACKWARDS;
	}
	else if ( cmd->buttons & IN_MOVELEFT ) {
		wish_direction = base.y + directions::LEFT;
	}
	else if ( cmd->buttons & IN_MOVERIGHT ) {
		wish_direction = base.y + directions::RIGHT;
	}
	else {
		// Reset to forward when not pressing a movement key, then fix anti-aim strafing by setting IN_FORWARD
		cmd->buttons |= IN_FORWARD;
		wish_direction = base.y + directions::FORWARDS;
	}
	
	// Calculate the ideal strafe rotation based on player speed (c) navewindre
	float speed_rotation = std::min( RAD2DEG( asinf( 30.f / localplayer->GetVelocity().Length2D( ) ) ) * 0.5f, 45.f );
	if ( in_transition ) {
		// Calculate the step by using our ideal strafe rotation
		float ideal_step = speed_rotation + calculated_direction;
		float step = fabsf( Math::ReturningNormalizeYaw( calculated_direction - ideal_step ) );
		
		// Check when the calculated direction arrives close to the wish direction
		if ( fabsf( Math::ReturningNormalizeYaw( wish_direction - calculated_direction ) ) > step ) {
			float add = Math::ReturningNormalizeYaw( calculated_direction + step );
			float sub = Math::ReturningNormalizeYaw( calculated_direction - step );
			
			// Step in direction that gets us closer to our wish direction
			if ( fabsf( Math::ReturningNormalizeYaw( wish_direction - add ) ) >= fabsf( Math::ReturningNormalizeYaw( wish_direction - sub ) ) ) {
				calculated_direction -= step;
			}
			else {
				calculated_direction += step;
			}
		}
		else {
			in_transition = false;
		}
	}
	else {
		// Get rough direction and setup calculated direction only when not transitioning
		float rough_direction = get_rough_direction( true_direction );
		calculated_direction = rough_direction;
		
		// When we have a difference between our current (rough) direction and our wish direction, then transition
		if ( rough_direction != wish_direction ) {
			in_transition = true;
		}
	}
	
	// Set movement up to be rotated
	cmd->forwardmove = 0.f;
	cmd->sidemove = cmd->command_number % 2 ? 450.f : -450.f;
	
	// Calculate ideal rotation based on our newly calculated direction
	float direction = ( cmd->command_number % 2 ? speed_rotation : -speed_rotation ) + calculated_direction;
	
	// Rotate our direction based on our new, defininite direction
	float rotation = DEG2RAD( base.y - direction );
	
	float cos_rot = cos( rotation );
	float sin_rot = sin( rotation );
	
	float forwardmove = ( cos_rot * cmd->forwardmove ) - ( sin_rot * cmd->sidemove );
	float sidemove = ( sin_rot * cmd->forwardmove ) + ( cos_rot * cmd->sidemove );
	
	// Apply newly rotated movement
	cmd->forwardmove = forwardmove;
	cmd->sidemove = sidemove;*/
}


void AutoStrafe::CreateMove(CUserCmd* cmd)
{
	if (!Settings::AutoStrafe::enabled)
		return;

	C_BasePlayer* localplayer = (C_BasePlayer*) entityList->GetClientEntity(engine->GetLocalPlayer());
	if (!localplayer)
		return;

	if (!localplayer->GetAlive())
		return;

	if (localplayer->GetMoveType() == MOVETYPE_LADDER || localplayer->GetMoveType() == MOVETYPE_NOCLIP)
		return;

	switch (Settings::AutoStrafe::type)
	{
		case AutostrafeType::AS_FORWARDS:
		case AutostrafeType::AS_BACKWARDS:
		case AutostrafeType::AS_LEFTSIDEWAYS:
		case AutostrafeType::AS_RIGHTSIDEWAYS:
			LegitStrafe(localplayer, cmd);
			break;
		case AutostrafeType::AS_RAGE:
			RageStrafe(localplayer, cmd);
			break;
	}
}
