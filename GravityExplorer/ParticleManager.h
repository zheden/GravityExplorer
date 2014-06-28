#pragma once

#include <iostream>


float randf(float a, float b) {
    float random = ((float) std::rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}


struct tParticle {
	float triangle[3][3];
	float color[4];
	float velocity[3];
	float position[3];
};

bool particlesCreatePending(false);
std::vector <tParticle> particles;

/* location is an array of 3 floats for the world coordinates we want the particles to start at */
void createParticles(float *location)
{
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	const int numParticles = 500;
	/* generate random values for the triangles */
	for(int n=0; n < numParticles; n++)
	{
		tParticle particle;

		for(int o=0; o < 3; o++)
		{
			for(int p=0; p < 3; p++)
			{
				particle.triangle[o][p] = location[p] + randf(-0.002, 0.002);
			}

			particle.velocity[o] = randf(-0.1, 0.1);
			particle.position[o] = 0.0;
		}
			
		particle.color[0] = randf(0.5, 1.0);
		particle.color[1] = randf(0.1, 0.6);
		particle.color[2] = 0;
		particle.color[3] = randf(0.5, 1.0);

		particles.push_back(particle);
	}
}

void drawParticles()
{
	for(int i=0; i < particles.size(); i++)
	{
		glColor4fv(particles[i].color);
		glPushMatrix();
		glTranslatef(particles[i].position[0], particles[i].position[1], particles[i].position[2]);
		
	glBegin(GL_TRIANGLES);
		//glVertex3f(particles[i].triangle[0][0] + particles[i].position[0], particles[i].triangle[0][1] + particles[i].position[1], particles[i].triangle[0][2] + particles[i].position[2]);
		glVertex3fv(particles[i].triangle[0]);
		glVertex3fv(particles[i].triangle[1]);
		glVertex3fv(particles[i].triangle[2]);
		
	glEnd();
		glPopMatrix();
	}
}

void updateParticles(double deltaTime)
{
	for (int i=0; i < particles.size(); i++)
	{
		for (int j=0; j < 3; j++)
		{
			particles[i].position[j] += particles[i].velocity[j] * deltaTime;
		}
	}
}