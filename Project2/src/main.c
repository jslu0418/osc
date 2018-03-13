#include "common.h"

/* four queue, one for all patients, other three for each doctor's patients*/
CIRCLEQ_HEAD(circleq, entry) head[4];
struct entry {
  CIRCLEQ_ENTRY(entry) entries;
  int id;
};

void
enqueue(int qid, int *id)
{
  struct entry *e = calloc(1, sizeof(struct entry));
  e->id = *id;
  CIRCLEQ_INSERT_TAIL(&head[qid], e, entries);
}

void
dequeue(int qid, int *id)
{
  struct entry *e = CIRCLEQ_FIRST(&head[qid]);
  *id = e->id;
  CIRCLEQ_REMOVE(&head[qid], e, entries);
}


/* Clinic Simulator */
sem_t nursecall[3];
sem_t nursetell[3];
sem_t patients;
sem_t symptoms[30];
sem_t finish_advising[30];
sem_t finish_registering[30];
sem_t leaves[30];
sem_t waitingfordoctor[3];
sem_t mutex1, mutex2;

/* doctor nurse thread arg struct */
struct d_thread_info {
  pthread_t t_id;
  int doctor_id;
};

/* patient thread arg struct */
struct p_thread_info {
  pthread_t t_id;
  int patient_id;
  int doctor_id;
};

/* receptionist thread arg struct */
struct r_thread_info {
  pthread_t t_id;
  int num_doctors;
  struct p_thread_info *pinfos;
};

static void *
receptionist(void *args)
{
  srand(time(NULL));
  struct r_thread_info *rinfo = args;
  int *patient_id = calloc(1, sizeof(int));
  int doctor_id;
  while(1)
    {
      /* waiting for patients */
      sem_wait(&patients);
      dequeue(0, patient_id);
      printf("Receptionist registers patient %d\n", *patient_id);
      sleep(1);
      /* register for next patient to random doctor */
      doctor_id = rand()%rinfo->num_doctors;
      enqueue(doctor_id + 1, patient_id);
      rinfo->pinfos[*patient_id].doctor_id = doctor_id;
      /* letting the patient know register has finished */
      sem_post(&finish_registering[*patient_id]);
    }
  return NULL;
}

static void *
doctor_and_nurse(void *args)
{
  struct d_thread_info *dinfo = args;
  int doctor_id = dinfo->doctor_id;
  int *patient_id = calloc(1, sizeof(int));
  while(1)
    {
      /* wait for patient looking help for this doctor */
      sem_wait(&waitingfordoctor[doctor_id]);
      dequeue(doctor_id + 1, patient_id);
      /* call next patient */
      sem_post(&nursecall[doctor_id]);
      printf("Nurse %d takes patient %d to doctor's office\n", doctor_id, *patient_id);
      sleep(1);
      /* directing patient to doctor's office */
      sem_wait(&nursetell[doctor_id]);
      /* listening to symptoms */
      sem_wait(&symptoms[*patient_id]);
      printf("Doctor %d listens to symptoms from patient %d\n", doctor_id, *patient_id);
      sleep(1);
      /* advising this patient */
      sem_post(&finish_advising[*patient_id]);
      /* wait for patient leaving */
      sem_wait(&leaves[*patient_id]);
    }
  return NULL;
}

static void *
patient(void *args)
{
  struct p_thread_info *pinfo = args;
  int *patient_id = &pinfo->patient_id;
  /* entering clinic */
  sem_wait(&mutex1);
  enqueue(0, patient_id);
  sem_post(&mutex1);
  sem_post(&patients);
  printf("Patient %d enters waiting room, waits for receptionist\n", pinfo->patient_id);
  sleep(1);
  /* finish registering */
  sem_wait(&finish_registering[*patient_id]);
  int doctor_id = pinfo->doctor_id;
  /* sitting in the waiting room */
  sem_post(&waitingfordoctor[doctor_id]);
  printf("Patient %d leaves receptionist and sits in waiting room\n", pinfo->patient_id);
  sleep(1);
  sem_wait(&nursecall[doctor_id]);
  /* walking to doctor's office */
  sleep(1);
  /* entering doctor's office */
  sem_post(&nursetell[doctor_id]);
  printf("Patient %d enters doctor %d's office\n", pinfo->patient_id, doctor_id);
  sleep(1);
  /* telling doctor's the symptoms */
  sem_post(&symptoms[pinfo->patient_id]);
  sem_wait(&finish_advising[pinfo->patient_id]);
  /* listening doctor's advice */
  printf("Patient %d receives advice from doctor %d\n", pinfo->patient_id, doctor_id);
  sleep(1);
  /* leave */
  printf("Patient %d leaves\n", pinfo->patient_id);
  sleep(1);
  sem_post(&leaves[pinfo->patient_id]);
}

int
main (int argc, char **argv)
{
  int num_doctors = atoi(argv[1]);
  int num_patients = atoi(argv[2]);
  int i;
  /* init queue */
  CIRCLEQ_INIT(&head[0]);
  CIRCLEQ_INIT(&head[1]);
  CIRCLEQ_INIT(&head[2]);
  CIRCLEQ_INIT(&head[3]);
  /* init all semaphores */
  for(i=0; i<num_doctors; i++)
    {
      sem_init(&nursecall[i], 0, 0);
      sem_init(&nursetell[i], 0, 0);
      sem_init(&waitingfordoctor[i], 0, 0);
    }
  sem_init(&patients, 0, 0);
  for(i=0; i<num_patients; i++)
    {
      sem_init(&symptoms[i], 0, 0);
      sem_init(&finish_advising[i], 0, 0);
      sem_init(&finish_registering[i], 0, 0);
      sem_init(&leaves[i], 0, 0);
    }
  sem_init(&mutex1, 0, 1);
  sem_init(&mutex2, 0, 1);

  /* init thread info*/
  struct r_thread_info r_info;
  struct d_thread_info *d_info = calloc(num_doctors, sizeof(struct d_thread_info));
  struct p_thread_info *p_info = calloc(num_patients, sizeof(struct p_thread_info));
  r_info.pinfos = p_info;
  r_info.num_doctors = num_doctors;
  pthread_create(&r_info.t_id, NULL, &receptionist, &r_info);
  for(i=0; i<num_doctors; i++)
    {
      d_info[i].doctor_id = i;
      pthread_create(&d_info[i].t_id, NULL, &doctor_and_nurse, &d_info[i]);
    }
  for(i=0; i<num_patients; i++)
    {
      p_info[i].patient_id = i;
      pthread_create(&p_info[i].t_id, NULL, &patient, &p_info[i]);
    }
  /* join every patient thread */
  for(i=0; i<num_patients; i++)
    {
      pthread_join(p_info[i].t_id, NULL);
    }
  pthread_cancel(r_info.t_id);
  for(i=0; i<num_doctors; i++)
    {
      pthread_cancel(d_info[i].t_id);
    }
  return 0;
}
