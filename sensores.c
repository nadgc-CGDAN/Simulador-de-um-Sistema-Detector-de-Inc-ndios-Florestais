#include "../headers/sensores.h"
#include "../headers/utils.h"

/* OUTPUT STANDARD FILE */
FILE *out;

/* THREADS VARIABLES */
pthread_cond_t sensor, fire, bomb;
pthread_mutex_t lock;

/* GLOBAL VARIABLE */
int has_fire = FALSE;
int calling_central = FALSE;
int extinguish_fire = FALSE;
int fire_extinguished = FALSE;


coord check_neigh_fire(coord c)
{
	coord fire_coords;
	if(area[c.x + 1][c.y].id == 'X'){ // LESTE
		fire_coords.x = c.x + 1;
		fire_coords.y = c.y;
	} else if (area[c.x - 1][c.y].id == 'X'){ // OESTE
		fire_coords.x = c.x - 1;
		fire_coords.y = c.y;
	} else if (area[c.x][c.y + 1].id == 'X'){ // NORTE
		fire_coords.x = c.x;
		fire_coords.y = c.y + 1;
	} else if (area[c.x][c.y - 1].id == 'X'){ // SUL
		fire_coords.x = c.x;
		fire_coords.y = c.y - 1;
	} else if (area[c.x - 1][c.y + 1].id == 'X'){ // NOROESTE
		fire_coords.x = c.x - 1;
		fire_coords.y = c.y + 1;
	} else if (area[c.x + 1][c.y + 1].id == 'X'){ // NORDESTE
		fire_coords.x = c.x + 1;
		fire_coords.y = c.y + 1;
	} else if (area[c.x - 1][c.y - 1].id == 'X'){ // SUDOESTE
		fire_coords.x = c.x - 1;
		fire_coords.y = c.y - 1;
	} else { // SUDESTE
		fire_coords.x = c.x + 1;
		fire_coords.y = c.y - 1;
	}

	return fire_coords;	
}

void *purge_fire(void *id)
{
	while (1)
	{
		pthread_mutex_lock(&lock);
		if (!extinguish_fire)
		{
			pthread_cond_wait(&bomb, &lock);
		}
		
		coord c = check_neigh_fire((*node_on_fire)->c);
		area[c.x][c.y].id = '-';

		sleep(2);
		extinguish_fire = FALSE;
		fire_extinguished = TRUE;
		has_fire = FALSE;
		pthread_mutex_unlock(&lock);
	}
}

void *call_central(void *id)
{
	while (1)
	{
		pthread_mutex_lock(&lock);
		if (!calling_central)
		{
			pthread_cond_wait(&sensor, &lock);
		}
		CT ct = get_time();
		coord c = check_neigh_fire((**node_on_fire).c);
		FILE* f = fopen("incendios.log", "a+");

		fprintf(f, "ID %s ([%d:%d:%d]): Incêndio nas coordenadas (%d, %d)\n", (**node_on_fire).hash, ct.hour, ct.min, ct.seconds, c.x, c.y);		
		fclose(f);

		if (((THREAD_NODE *)&node_on_fire)->isBorder)
		{
			pthread_cond_signal(&bomb);
		}

		THREAD_NODE **next = (*node_on_fire)->vizinhos;
		while (!(**next).isBorder)
		{
			int n = (**next).qtd_vizinhos;
			if (n > 0)
			{
				int min_value = __INT_MAX__;
				int index_of_min_value = -1;
				for(int i = 0; i < n; i++){
					int x_vizinho = (*(**next).vizinhos[i]).c.x;
					if(min_value > x_vizinho){
						min_value = x_vizinho;
						index_of_min_value = i;
					}
				}
				next = &((*next)->vizinhos[index_of_min_value]);
				assert(next != NULL);
			}

			sleep(2);
		}
		
		extinguish_fire = TRUE;
		calling_central = FALSE;
		pthread_cond_signal(&bomb);
		pthread_mutex_unlock(&lock);
	}
}

static void cleanup_handler(void *t) 
{
	pthread_mutex_unlock(&lock);
}

void *check_fire(void *th)
{
	THREAD_NODE *t = (struct THREAD_NODE *)th;
	pthread_cleanup_push(cleanup_handler, t); 

	while (1)
	{
		pthread_mutex_lock(&lock);
		if (!fire_extinguished)
		{
			pthread_cond_wait(&fire, &lock);
		}
		if ((area[t->c.x + 1][t->c.y].id == 'X' || area[t->c.x - 1][t->c.y].id == 'X' || area[t->c.x][t->c.y + 1].id == 'X' || area[t->c.x][t->c.y - 1].id == 'X') ||
			area[t->c.x - 1][t->c.y + 1].id == 'X' || area[t->c.x + 1][t->c.y + 1].id == 'X' || area[t->c.x - 1][t->c.y - 1].id == 'X' || area[t->c.x + 1][t->c.y - 1].id == 'X')
		{
			sleep(1);
			node_on_fire = &t;
			calling_central = TRUE;
			fire_extinguished = FALSE;
			pthread_cond_signal(&sensor);
		}
		else
		{
			pthread_cond_signal(&fire);
			pthread_cond_wait(&fire, &lock);
		}
		pthread_mutex_unlock(&lock);
	}
	pthread_cleanup_pop(1);
}

void create_threads(int size)
{
	pthread_cond_init(&sensor, NULL);
	pthread_cond_init(&fire, NULL);
	pthread_cond_init(&bomb, NULL);

	pthread_t central, bombeiro;

	pthread_create(&central, NULL, call_central, NULL);
	pthread_create(&bombeiro, NULL, purge_fire, NULL);

	for (int i = 0; i < size; i++)
	{
		pthread_create(&threads[i].thread, NULL, check_fire, (void *)&threads[i]);
	}
	/*
	for(int i = 0; i < size; i++){
		pthread_join(threads[i].thread, NULL);
	}
	*/
}

void destroy_threads()
{
	pthread_mutex_destroy(&lock);
	pthread_cond_destroy(&sensor);
	pthread_cond_destroy(&fire);
	pthread_cond_destroy(&bomb);

	fclose(out);
}

void put_fire()
{
	sleep(1);
	//if (!has_fire)
	{
		coord c;
		c.x = rand() % WIDTH;
		c.y = rand() % HEIGHT;
		if (area[c.x][c.y].id == 'T')
		{
			area[c.x][c.y].t->isUp = FALSE;
			area[c.x][c.y].id = 'X';
			pthread_cancel(area[c.x][c.y].t->thread);
			pthread_join(area[c.x][c.y].t->thread, NULL);
		}
		else
		{
			area[c.x][c.y].id = 'X';
			has_fire = 1;
		}
		pthread_cond_signal(&fire);

	}
}

void print_border(int size)
{
	for (int i = 0; i < size; i++)
	{
		if (threads[i].isBorder)
			printf("THREAD DA BORDA (%d, %d)\n", threads[i].c.x, threads[i].c.y);
	}
}

void create_neigh(int size)
{
	assert(size >= 2);
	for (int i = 0; i < size; i++)
	{

		if (((threads[i].c.x - 1 == 0) || (threads[i].c.x + 1 == HEIGHT - 1)) || ((threads[i].c.y + 1 == WIDTH - 1) || (threads[i].c.y - 1 == 0)))
		{
			threads[i].isBorder = TRUE;
		}
		if ((threads[i].c.x - 1 == 0) && (threads[i].isBorder == TRUE))
		{

			if (threads[i].c.y - 1 == 0)
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 2);
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].qtd_vizinhos = 2;
				threads[i].vizinhos[0] = area[threads[i].c.x + 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y + 3].t;
			}
			else if (threads[i].c.y == WIDTH - 2)
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 2);
				threads[i].qtd_vizinhos = 2;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[0] = area[threads[i].c.x + 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y - 3].t;
			}
			else
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 3);
				threads[i].qtd_vizinhos = 3;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[2] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

				threads[i].vizinhos[0] = area[threads[i].c.x + 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y + 3].t;
				threads[i].vizinhos[2] = area[threads[i].c.x][threads[i].c.y - 3].t;
			}
		}
		else if ((threads[i].c.x == HEIGHT - 2) && (threads[i].isBorder == TRUE))
		{
			if (threads[i].c.y - 1 == 0)
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 2);
				threads[i].qtd_vizinhos = 2;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

				threads[i].vizinhos[0] = area[threads[i].c.x - 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y + 3].t;
			}
			else if (threads[i].c.y == WIDTH - 2)
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 2);
				threads[i].qtd_vizinhos = 2;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

				threads[i].vizinhos[0] = area[threads[i].c.x - 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y - 3].t;
			}
			else
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 3);
				threads[i].qtd_vizinhos = 3;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[2] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

				threads[i].vizinhos[0] = area[threads[i].c.x - 3][threads[i].c.y].t;

				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y + 3].t;
				threads[i].vizinhos[2] = area[threads[i].c.x][threads[i].c.y - 3].t;
			}
		}
		else
		{
			if (threads[i].c.y - 1 == 0 && threads[i].isBorder == TRUE)
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 3);
				threads[i].qtd_vizinhos = 3;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[2] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

				threads[i].vizinhos[0] = area[threads[i].c.x - 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y + 3].t;
				threads[i].vizinhos[2] = area[threads[i].c.x + 3][threads[i].c.y].t;
			}
			if (threads[i].c.y == WIDTH - 2 && threads[i].isBorder == TRUE)
			{
				threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 3);
				threads[i].qtd_vizinhos = 3;
				threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				threads[i].vizinhos[2] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

				threads[i].vizinhos[0] = area[threads[i].c.x - 3][threads[i].c.y].t;
				threads[i].vizinhos[1] = area[threads[i].c.x][threads[i].c.y - 3].t;
				threads[i].vizinhos[2] = area[threads[i].c.x + 3][threads[i].c.y].t;
			}
		}
	}

	int k = 3;

	for (int i = 0; i < size; i++)
	{
		int isBorder = threads[i].isBorder;

		if (!isBorder)
		{
			int x = threads[i].c.x;
			int y = threads[i].c.y;

			threads[i].vizinhos = (THREAD_NODE **)malloc(sizeof(THREAD_NODE *) * 4);

			threads[i].vizinhos[0] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
			threads[i].vizinhos[1] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
			threads[i].vizinhos[2] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
			threads[i].vizinhos[3] = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));

			threads[i].qtd_vizinhos = 0;

			if ((x != 1 || x != HEIGHT - 2))
			{
				threads[i].vizinhos[0] = area[x - k][y].t;
				threads[i].vizinhos[1] = area[x + k][y].t;
				threads[i].qtd_vizinhos += 2;
			}

			if ((y != 1 || y != WIDTH - 2))
			{
				threads[i].vizinhos[2] = area[x][y + k].t;
				threads[i].vizinhos[3] = area[x][y - k].t;
				threads[i].qtd_vizinhos += 2;
			}
		}
	}
}

void print_neigh()
{
	for (int i = 0; i < MAX_THREADS; i++)
	{
		printf("Qtd de vizinhos da thread %d: %d\n", i, threads[i].qtd_vizinhos);
		printf("Localidade da thread %d: (%d, %d)\n", i, threads[i].c.x, threads[i].c.y);
		for (int j = 0; j < threads[i].qtd_vizinhos; j++)
		{
			printf("\t(%d, %d)\n", threads[i].vizinhos[j]->c.x, threads[i].vizinhos[j]->c.y);
		}
		printf("\n");
	}
}

void create_area()
{
	int thread_node = FALSE;
	int i, k, j, t;
	int it_t = 0;
	for (i = 0, k = 1; i < WIDTH; i++)
	{
		if (i == k)
		{
			thread_node = TRUE;
			k += 3;
		}
		else
		{
			thread_node = FALSE;
		}
		for (j = 0, t = 1; j < HEIGHT; j++)
		{
			if (j == t && thread_node)
			{
				threads[it_t].c.x = i;
				threads[it_t].c.y = j;
				threads[it_t].isUp = TRUE;
				threads[it_t].hash = malloc(sizeof(MAX_WORD_LENGTH));
				randID(threads[it_t].hash);
				area[i][j].id = 'T';
				area[i][j].t = (THREAD_NODE *)malloc(sizeof(THREAD_NODE));
				area[i][j].t = &threads[it_t];

				it_t++;
				t += 3;
			}
			else
			{
				area[i][j].id = '-';
				area[i][j].t = NULL;
			}
		}
	}
	create_neigh(it_t);
	create_threads(it_t);
}

void print_area()
{
	int i, j, k = 4;

	while(k != 0){
		printf(GREEN(" "));
		k--;
	}
	for(k = 0; k < HEIGHT; k++){
		if(k < 10) 
			printf(GREEN("%d  "), k);
		else {
			if(k == 29)
				printf(GREEN("%d"), k);
			else
				printf(GREEN("%d "), k);
		}
	}
	printf("\n");
	for (i = 0; i < WIDTH; i++)
	{
		if(i < 10) printf(GREEN("%d  "), i);
		else printf(GREEN("%d "), i);
		
		for (j = 0; j < HEIGHT; j++)
		{
			if (area[i][j].id == '-' || area[i][j].id == 'T')
				printf(GREEN(" %c "), area[i][j].id);
			else if (area[i][j].id == 'X')
				printf(RED(" @ "));
		}
		printf("\n");
	}
}