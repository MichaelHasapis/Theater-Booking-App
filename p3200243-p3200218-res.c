#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "p3200243-p3200218-res.h"


unsigned int base;
struct timespec t11,t12,t21,t22,t31,t32;
pthread_cond_t      availability_caller ;
pthread_cond_t      availability_cashier ;
pthread_mutex_t     cashiers_mutex ;
pthread_mutex_t     caller_mutex ;
pthread_mutex_t     seats_mutex ;
pthread_mutex_t     timetofinm;
pthread_mutex_t     timetowaitm;
pthread_mutex_t     percentm;
pthread_mutex_t     accm;

int slot=0;
int slot2=0;
int zone_of_seats [30][10];


int success_rate=0;
int payment_fail_rate=0;
int unavailable_seats_rate=0;

int time_of_wait=0;
int time_of_finish=0;

void *Buying_phase(void* cust_id){
    
    
    int id = *(int *)cust_id;
        
    clock_gettime(CLOCK_REALTIME, &t11);

    pthread_mutex_lock(&caller_mutex);

    while(slot==Ntel){
        printf("All callers are unavailable at the moment please wait customer %d. \n", id);
        pthread_cond_wait(&availability_caller,&caller_mutex);
    }
    printf("Caller is available to help you customer %d.\n" ,id);
    
    pthread_mutex_lock(&timetowaitm);
    clock_gettime(CLOCK_REALTIME, &t12);
    time_of_wait+=(t12.tv_sec - t11.tv_sec);
    pthread_mutex_unlock(&timetowaitm);
    
    slot++;

    char *picked_zone;
    int zone=((rand_r(&base)%100)+1);

    if(zone>(int)(Pzonea*100)){
        picked_zone="B";
    }else{
        picked_zone="A";
    }

    
    pthread_mutex_lock(&seats_mutex);

    int num_to_seat=rand_r(&base)%(Nseathigh-Nseatlow+1)+Nseatlow;
    int secs=(rand_r(&base)%(tseathigh-tseatlow+1))+tseatlow;
    sleep(secs);


    int starting_spot[2];
    int booked=0;    //false=0
    int i,limit;
    //fix
    if(picked_zone=="B"){
        i=10;
        limit=30;
    }else{
        i=0;
        limit=10;
    }
    while(i<limit){
        int bookable=0;
        int j=0;
    starting_spot[1]=j;
    while(j<10){
	if(zone_of_seats[i][j]==0){
	    bookable++;
	}else{
	    starting_spot[1]=j+1;
	    bookable=0;
	}
	if(bookable==num_to_seat){
	    booked=1;
	    break;
	}
	j++;
    }
    if(booked==1){
	starting_spot[0]=i;
	break;
    }
    i++;
}
    if(booked==1){
        for(int c =starting_spot[1];c <starting_spot[1]+num_to_seat; c++){
	    zone_of_seats[starting_spot[0]][c]=id;
    }
}

    if(booked==0){
        printf("There are no available seats in the zone you asked customer %d.\n",id);
        
        pthread_mutex_lock(&timetofinm);
        clock_gettime(CLOCK_REALTIME, &t32);
        time_of_finish+=(t32.tv_sec - t11.tv_sec);
        pthread_mutex_unlock(&timetofinm);

        pthread_mutex_lock(&percentm);
        unavailable_seats_rate++;
        pthread_mutex_unlock(&percentm);
        pthread_mutex_unlock(&seats_mutex);
        slot--;

        pthread_cond_signal(&availability_caller);
        pthread_mutex_unlock(&caller_mutex);
        pthread_exit(NULL);
    }

    pthread_mutex_unlock(&seats_mutex);
    slot--;

    pthread_cond_signal(&availability_caller);
    pthread_mutex_unlock(&caller_mutex);
    
    printf("Customer %d proceeds to cashiers\n", id);
    

    clock_gettime(CLOCK_REALTIME, &t21);
    
    pthread_mutex_lock(&cashiers_mutex);

    while(slot2==Ncash){
        printf("All cashiers are unavailable at the moment please wait customer %d. \n", id);
        pthread_cond_wait(&availability_cashier,&cashiers_mutex);
    }

    pthread_mutex_lock(&timetowaitm);
    clock_gettime(CLOCK_REALTIME, &t22);
    time_of_wait+=(t22.tv_sec - t21.tv_sec);
    pthread_mutex_unlock(&timetowaitm);

    printf("Cashier helps customer %d\n",id);
    slot2++;
    sleep((rand_r(&base)%(tcashhigh-tcashlow+1))+tcashlow);

    int accepted;
    accepted=(rand_r(&base)%100)+1;
    
    
    
    pthread_mutex_lock(&seats_mutex);
    
	
    if(accepted<=(Pcardsucces*100)){
        
        int payment;
        if(picked_zone=="B"){
            payment= num_to_seat*Czoneb;
        }else{
            payment= num_to_seat*Czonea;
            
        }
	
        pthread_mutex_lock(&accm);
        comp_account+=payment;
        pthread_mutex_unlock(&accm);

	
        //print seats and zone
        printf("Successful booking. Your seats are in zone %c",*picked_zone);
        printf(",Row %d",starting_spot[0]+1);
        for(int d=0;d<num_to_seat;d++){
            printf(",Seat %d" ,(starting_spot[1]+d+1));
        }
        printf(",with a total cost of %d ", payment);
        printf("customer %d \n", id);
        pthread_mutex_lock(&percentm);
        success_rate++;
        pthread_mutex_unlock(&percentm);

    }else{
        
        for(int d=0;d<num_to_seat;d++){
            zone_of_seats[starting_spot[0]][starting_spot[1]+d]=0;
        }
        //release seats
        printf("There was an error in payment customer %d.\n", id);

        pthread_mutex_lock(&percentm);
        payment_fail_rate++;
        pthread_mutex_unlock(&percentm);

    }

    pthread_mutex_lock(&timetofinm);
    clock_gettime(CLOCK_REALTIME, &t31);
    time_of_finish+=(t31.tv_sec - t11.tv_sec);
    pthread_mutex_unlock(&timetofinm);

    pthread_mutex_unlock(&seats_mutex);
    slot2--;
    pthread_cond_signal(&availability_cashier);
    pthread_mutex_unlock(&cashiers_mutex);

    
}


void *Start_Buying(void *amount){
    
    int cust;
    int *count = (int *) amount;
    printf("%d\n",*count);
    int count_cust [*count];
    
    pthread_t *threads = malloc((*count) * sizeof(pthread_t));

    for(int i=0;i<*count;i++){
        count_cust[i]=i+1;

        if(i!=0){
            sleep((rand_r(&base)%(treshigh-treslow+1))+treslow);
        }
        cust = pthread_create(&threads[i],NULL,Buying_phase, &count_cust[i]);

    }


    for(int i=0;i<*count;i++){ 
        cust = pthread_join(threads[i],NULL);
    }

    char *zoner;
    for(int i=0;i<30;i++){
        if(i<10){
            zoner="A";
        }else{
            zoner="B";
        }
        for(int j=0;j<10;j++){
            if(zone_of_seats[i][j]!=0){
                printf("Zone %c",*zoner);
                printf("/Row %d",i+1);
                printf("/Seat %d",j+1);
                printf("/Customer %d /", zone_of_seats[i][j]);
            }
        }
    }

    printf("Total earning: %d \n",comp_account);
    printf("Total succes rate: %f \n",success_rate/((float)*count));
    printf("Total unavailable seats rate: %f \n",unavailable_seats_rate/((float)*count));
    printf("Total payment fail rate: %f \n",payment_fail_rate/((float)*count));
    printf("Average time of wait: %f \n",time_of_wait/((float)*count));
    printf("Average time till finish: %f \n",time_of_finish/((float)*count));

    for(int m=0;m<30;m++){
    	for(int n=0;n<10;n++){
    		printf("|%.3d|",zone_of_seats[m][n]);
    	}
    	printf("\n");
    }


    free(threads);

}


int main(int argc, char *argv[]){
    if(argc !=3){
        printf("An error has occured: Wrong amount of arguments!");
        return -1;
    }

    int amount_of_customers = atoi(argv[1]);
    if(amount_of_customers<0){
        printf("An error has occured: Amount of customers must be non-negative!");
        return -2;
    }
    base=atoi(argv[2]);

    pthread_mutex_init(&caller_mutex, NULL);
    pthread_mutex_init(&cashiers_mutex, NULL);
    pthread_mutex_init(&seats_mutex, NULL);
    pthread_mutex_init(&timetofinm, NULL);
    pthread_mutex_init(&timetowaitm, NULL);
    pthread_mutex_init(&percentm, NULL);
    pthread_mutex_init(&accm, NULL);
    pthread_cond_init(&availability_caller, NULL);
    pthread_cond_init(&availability_cashier, NULL);

    pthread_t starting_thread;

    pthread_create(&starting_thread,NULL, Start_Buying , &amount_of_customers);
    
    pthread_join(starting_thread,NULL);
    
    pthread_mutex_destroy(&caller_mutex);
    pthread_mutex_destroy(&cashiers_mutex);
    pthread_mutex_destroy(&seats_mutex);
    pthread_mutex_destroy(&timetofinm);
    pthread_mutex_destroy(&timetowaitm);
    pthread_mutex_destroy(&percentm);
    pthread_mutex_destroy(&accm);
    pthread_cond_destroy(&availability_caller);
    pthread_cond_destroy(&availability_cashier);


    return 0;
}