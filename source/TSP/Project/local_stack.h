#ifndef _LOCAL_STACK_H_
#define _LOCAL_STACK_H_

#define LOCAL_STACK_SIZE 500

struct route;

struct local_stack {
	struct route stack[LOCAL_STACK_SIZE];
	int top_index; // Index of the top element
	int bottom_index; // Index of the bottom element
};

void init_stack(struct local_stack *s) {
	s->top_index = 0;
	s->bottom_index = 0;
}

int push(struct local_stack *s, struct route *c) {
	if ((s->top_index + 1) % LOCAL_STACK_SIZE == s->bottom_index) {
		// Stack is full
		printf("[][!!!] push(): Stack is full!\n");
		return 0;
	}
	s->stack[s->top_index] = *c;
	s->top_index = (s->top_index + 1) % LOCAL_STACK_SIZE;
	return 1;
}

int pop(struct local_stack *s, struct route *c) {
	if (s->top_index == s->bottom_index) {
		// Stack is empty
		return 0;
	}
	s->top_index = (s->top_index + LOCAL_STACK_SIZE - 1) % LOCAL_STACK_SIZE;
	*c = s->stack[s->top_index];
	return 1;
}

int pop_rear(struct local_stack *s, struct route *c) {
	if (s->top_index == s->bottom_index) {
		// Stack is empty
		return 0;
	}
	*c = s->stack[s->bottom_index];
	s->bottom_index = (s->bottom_index + 1) % LOCAL_STACK_SIZE;
	return 1;
}

int peek_rear(struct local_stack *s, struct route *c) {
	if (s->top_index == s->bottom_index) {
		// Stack is empty
		return 0;
	}
	*c = s->stack[s->bottom_index];
	return 1;
}

int empty(struct local_stack *s) {
	if (s->top_index == s->bottom_index) {
		// Stack is empty
		return 1;
	}
	return 0; // Stack is not empty
}

#endif // ifndef _LOCAL_STACK_H_
