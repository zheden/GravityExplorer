#pragma once

#include <iostream>

float randf(float a, float b) {
    float random = ((float) std::rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

enum ParticleType {
	PARTICLE_FLYING,
	PARTICLE_STRETCHING
};

enum StretchPhase {
	PHASE_STRETCH,
	PHASE_SHRINK
};

struct Particle {
	ParticleType type;

	float triangle[3][3];
	
	float color[4];

	float initVelocity[3];
	float velocity[3];
	float position[3];
	float damping;
	
	float life;
	float lifespan;

	// phase of stretching particles
	StretchPhase phase;
	float stretchPhasePercent;

	bool active;
};

bool particlesCreatePending(false);
std::vector <Particle> particles;

/**
*	location - an array of 3 floats for the world coordinates we want the particles to start at 
*/
void AddParticles(float location[3], int numParticles, ParticleType type, float speed)
{
	for(int i = 0; i < numParticles; i++)
	{
		Particle particle;

		particle.type = type;

		/* init a flying particle */
		if (type == PARTICLE_FLYING)
		{
			for(int j = 0; j < 3; j++)
			{
				for(int k = 0; k < 3; k++)
				{
					particle.triangle[j][k] = location[k] + randf(-0.002, 0.002);
				}

				particle.initVelocity[j] = randf(-speed, speed);
				particle.position[j] = 0.0;
			}

			particle.lifespan = 10;
			particle.active = true;
			particle.damping = 0.8;
		}

		/* init a stretching particle */
		if (type == PARTICLE_STRETCHING)
		{
			for(int j = 0; j < 3; j++)
			{
				for(int k = 0; k < 3; k++)
				{
					particle.triangle[j][k] = location[k] + randf(-0.001, 0.001);
				}

				particle.initVelocity[j] = randf(-speed, speed);
				particle.position[j] = 0.0;
			}

			particle.lifespan = 10;
			particle.life = 0;
			particle.active = true;
			particle.phase = PHASE_STRETCH;
			particle.stretchPhasePercent = 0.25;
			particle.damping = 0.75;
		}

		particle.velocity[0] = particle.initVelocity[0];
		particle.velocity[1] = particle.initVelocity[1];
		particle.velocity[2] = particle.initVelocity[2];

		particle.color[0] = randf(0.5, 1.0);
		particle.color[1] = randf(0.1, 0.6);
		particle.color[2] = 0;
		particle.color[3] = randf(0.5, 1.0);

		particles.push_back(particle);
	}
}

void DrawParticles()
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(int i=0; i < particles.size(); i++)
	{
		Particle p = particles[i];

		if (!p.active)
		{
			continue;
		}

		glPushMatrix();
		glColor4fv(p.color);

		if (p.type == PARTICLE_FLYING)
		{
			glTranslatef(p.position[0], p.position[1], p.position[2]);
		}

		glBegin(GL_TRIANGLES);

		glVertex3fv(p.triangle[0]);
		glVertex3fv(p.triangle[1]);
		glVertex3fv(p.triangle[2]);
		
		glEnd();
		
		glPopMatrix();
	}

   glDisable(GL_BLEND);
}

void updateParticles(double deltaTime)
{
	Particle* p;

	for (int i=0; i < particles.size(); i++)
	{
		p = &particles[i];

		for (int j=0; j < 3; j++)
		{
			// move whole triangle
			if (p->type == PARTICLE_FLYING)
			{
				p->position[j] += p->velocity[j] * deltaTime;
			}

			// move vertices
			if (p->type == PARTICLE_STRETCHING)
			{
				// If the particle is in the stretching phase
				if (p->phase == PHASE_STRETCH)
				{
					// move vertex 0
					p->triangle[0][j] += p->velocity[j] * deltaTime;
				}
				// The particle is in the shrinking stage
				else
				{
					// move vertexes 1 and 2
					p->triangle[1][j] += p->velocity[j] * deltaTime;
					p->triangle[2][j] += p->velocity[j] * deltaTime;
				}
			}

			// damping
			p->velocity[j] *= p->damping;

			// fade out
			p->color[3] -= 0.15 * deltaTime;

			// decrease life left
			p->life += deltaTime;

			// deactivate if dead
			if (p->life >= p->lifespan)
			{
				p->active = false;
			}

			// check if need to change phase on this frame
			if (p->type == PARTICLE_STRETCHING && p->phase == PHASE_STRETCH && p->life >= p->stretchPhasePercent * p->lifespan)
			{
				p->phase = PHASE_SHRINK;

				// reset velocity
				p->velocity[0] = p->initVelocity[0];
				p->velocity[1] = p->initVelocity[1];
				p->velocity[2] = p->initVelocity[2];
			}
		}
	}
}