#pragma once

#include <iostream>


float randf(float a, float b) {
    float random = ((float) std::rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

enum ParticleType {
	FLYING,
	STRETCHING
};

struct tParticle {
	ParticleType type;

	float triangle[3][3];
	
	float color[4];

	float velocity[3];
	float position[3];
	
	float life;
	float lifespan;
	bool active;
};

bool particlesCreatePending(false);
std::vector <tParticle> particles;

/* location is an array of 3 floats for the world coordinates we want the particles to start at */
void createParticles(float location[3])
{
	const int numParticles = 500;
	/* create flying particles */
	for(int i = 0; i < numParticles; i++)
	{
		tParticle particle;

		particle.type = FLYING;

		for(int o = 0; o < 3; o++)
		{
			for(int p = 0; p < 3; p++)
			{
				particle.triangle[o][p] = location[p] + randf(-0.002, 0.002);
			}

			particle.velocity[o] = randf(-0.1, 0.1);
			particle.position[o] = 0.0;
		}

		particle.lifespan = 10;
		particle.active = true;
		
		particle.color[0] = randf(0.5, 1.0);
		particle.color[1] = randf(0.1, 0.6);
		particle.color[2] = 0;
		particle.color[3] = randf(0.5, 1.0);

		particles.push_back(particle);
	}
	
	/* create stretching particles */
	for(int i = 0; i < numParticles; i++)
	{
		tParticle particle;
		
		particle.type = STRETCHING;

		for(int o = 0; o < 3; o++)
		{
			for(int p = 0; p < 3; p++)
			{
				particle.triangle[o][p] = location[p] + randf(-0.001, 0.001);
			}

			particle.velocity[o] = randf(-0.3, 0.3);
			particle.position[o] = 0.0;
		}

		particle.lifespan = 5;
		particle.life = 0;
		particle.active = true;
		
		particle.color[0] = randf(0.5, 1.0);
		particle.color[1] = randf(0.1, 0.6);
		particle.color[2] = 0;
		particle.color[3] = randf(0.5, 1.0);

		particles.push_back(particle);
	}
}

void drawParticles()
{
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	for(int i=0; i < particles.size(); i++)
	{
		tParticle p = particles[i];

		if (!p.active)
		{
			continue;
		}

		glPushMatrix();
		glColor4fv(p.color);

		if (p.type == FLYING)
		{
			glTranslatef(p.position[0], p.position[1], p.position[2]);
		}
		
		glBegin(GL_TRIANGLES);
		
		if (p.type == STRETCHING)
		{
			// If the particle is in the stretching phase
			if (p.life < 2)
			{
				particles[i].triangle[0][0] += particles[i].position[0];
				particles[i].triangle[0][1] += particles[i].position[1];
				particles[i].triangle[0][2] += particles[i].position[2];
			}
			// The particle is in the shrinking stage
			else
			{
				particles[i].triangle[1][0] += particles[i].position[0];
				particles[i].triangle[1][1] += particles[i].position[1];
				particles[i].triangle[1][2] += particles[i].position[2];
				
				particles[i].triangle[2][0] += particles[i].position[0];
				particles[i].triangle[2][1] += particles[i].position[1];
				particles[i].triangle[2][2] += particles[i].position[2];
			}
		}
		
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
	for (int i=0; i < particles.size(); i++)
	{
		for (int j=0; j < 3; j++)
		{
			particles[i].position[j] += particles[i].velocity[j] * deltaTime;

			particles[i].color[3] -= 0.15 * deltaTime;

			particles[i].life += deltaTime;

			if (particles[i].life >= particles[i].lifespan)
			{
				particles[i].active = false;
			}
		}
	}
}