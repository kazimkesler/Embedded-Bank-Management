#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
typedef struct
{
	unsigned int accid;
	char name[16];
	char surname[16];
	double balance;
} Account;

typedef struct
{
	unsigned int lastid;
	bool success;
} Confing;

bool open_file(FILE**, const char*, const char*);
char file_check(const char*);
bool check_files();
bool save_last_id(Confing);
Confing get_last_id();
unsigned int insert_customer(Account*);
unsigned int delete_customer(unsigned int);
unsigned int add_money(unsigned int, double);
Account* select_customer(unsigned int);
bool list_all(bool);

bool open_file(FILE** fpt, const char* name, const char* mode)
{
	*fpt = fopen(name, mode);
	return *fpt == NULL ? false : true;
}
char file_check(const char* name)
{
	FILE* fpt;
	char state = open_file(&fpt, name, "rb");
	if (!state)
	{
		state = open_file(&fpt, name, "wb");
		if (state) state = -1;
	}
	if (state)
		fclose(fpt);
	return state;
}
bool check_files()
{
	char customers, confings;
	customers = file_check("customers.dat");
	confings = file_check("confings.dat");
	if (customers && confings)
	{
		if (confings < 0 || customers < 0)
		{
			FILE* fpt;
			if (confings > 0)
			{
				open_file(&fpt, "confings.dat", "wb");
				fclose(fpt);
			}
			if (customers > 0)
			{
				open_file(&fpt, "customers.dat", "wb");
				fclose(fpt);
			}
			Confing cfg = { 0, true };
			if (!save_last_id(cfg))
				return false;
		}
		return true;
	}
	else
		return false;
}
bool save_last_id(Confing cfg)
{
	FILE* fpt;
	bool success = false;
	if (open_file(&fpt, "confings.dat", "rb+"))
	{
		if (!fseek(fpt, 0, SEEK_SET))
			success = fwrite(&cfg, sizeof(cfg), 1, fpt);
		fclose(fpt);
	}
	return success;
}
Confing get_last_id()
{
	FILE* fpt;
	Confing cfg = { 0, false };
	bool success = false;
	if (open_file(&fpt, "confings.dat", "rb"))
	{
		if (!fseek(fpt, 0, SEEK_SET))
			success = fread(&cfg, sizeof(cfg), 1, fpt);
		fclose(fpt);
	}
	return cfg;
}
unsigned int insert_customer(Account* acc)
{
	bool state = false;
	Confing cfg = get_last_id();
	if (cfg.success)
		cfg.lastid++;
	if (cfg.lastid)
	{
		cfg.success = false;
		if (save_last_id(cfg))
		{
			acc->accid = cfg.lastid;
			FILE* fpt;
			if (open_file(&fpt, "customers.dat", "rb+"))
			{
				if (!fseek(fpt, (acc->accid - 1) * sizeof(Account), SEEK_SET))
				{
					if (fwrite(acc, sizeof(Account), 1, fpt))
					{
						cfg.success = true;
						save_last_id(cfg);
						state = true;
					}
				}
				fclose(fpt);
			}
		}
	}
	return state ? acc->accid : 0;
}
unsigned int delete_customer(unsigned int id)
{
	FILE* fpt;
	bool state = false;
	unsigned int deleted;
	Confing cfg = get_last_id();
	if ((cfg.lastid -= !cfg.success) > 0 && id <= cfg.lastid)
	{
		if (open_file(&fpt, "customers.dat", "rb+"))
		{
			if (!fseek(fpt, (id - 1) * sizeof(Account), SEEK_SET))
			{
				deleted = id;
				if (fread(&id, sizeof(id), 1, fpt) && id)
				{
					fseek(fpt, (id - 1) * sizeof(Account), SEEK_SET);
					id = 0;
					if (fwrite(&id, sizeof(id), 1, fpt))
						state = true;
				}
			}
			fclose(fpt);
		}
	}
	return state ? deleted : 0;
}
unsigned int add_money(unsigned int id, double amount)
{
	FILE* fpt;
	Account acc;
	bool state = false;
	Confing cfg = get_last_id();
	if ((cfg.lastid -= !cfg.success) > 0 && id <= cfg.lastid)
	{
		if (open_file(&fpt, "customers.dat", "rb+"))
		{
			if (!fseek(fpt, (id - 1) * sizeof(Account), SEEK_SET))
			{
				if (fread(&acc, sizeof(Account), 1, fpt) && acc.accid)
				{
					acc.balance += amount;
					fseek(fpt, (id - 1) * sizeof(Account), SEEK_SET);
					if (fwrite(&acc, sizeof(Account), 1, fpt))
						state = true;
				}
			}
			fclose(fpt);
		}
	}
	return state ? id : 0;
}
Account* select_customer(unsigned int id)
{
	FILE* fpt;
	static Account acc;
	bool state = false;
	Confing cfg = get_last_id();
	if ((cfg.lastid -= !cfg.success) > 0 && id <= cfg.lastid)
	{
		if (open_file(&fpt, "customers.dat", "r"))
		{
			if (!fseek(fpt, (id - 1) * sizeof(Account), SEEK_SET))
				if (fread(&acc, sizeof(Account), 1, fpt) && acc.accid)
					state = true;
			fclose(fpt);
		}
	}
	return state ? &acc : NULL;
}
bool list_all(bool withdel)
{
	FILE* fpt, * fpt_out;
	bool state = false;
	Account acc;
	Confing cfg = get_last_id();
	if ((cfg.lastid -= !cfg.success) > 0)
	{
		if (open_file(&fpt, "customers.dat", "rb"))
		{
			if (open_file(&fpt_out, "list.txt", "w"))
			{
				if (fprintf(fpt_out, "%-16s%-16s%-16s%-16s\n", "ID", "NAME", "SURNAME", "BALANCE"))
				{
					size_t i;
					for (i = 0; i < cfg.lastid; i++)
						if (!fseek(fpt, i * sizeof(Account), SEEK_SET))
							if (fread(&acc, sizeof(Account), 1, fpt))
							{
								if (acc.accid || (!acc.accid && withdel))
									if (!fprintf(fpt_out, "%-16u%-16s%-16s%-14.2lf\n", acc.accid, acc.name, acc.surname, acc.balance))
										break;
							}
							else
								break;
						else break;
					if (i == cfg.lastid)
						state = true;
				}
				fclose(fpt_out);
			}
			fclose(fpt);
		}
	}
	return state;
}
int main()
{
	puts("-------BANK SYSTEM-------");
	if (check_files())
		puts("files have checked.");
	else
	{
		puts("files dont exist.");
		exit(1);
	}
	char input[48] = { 0 }, cmd[16] = { 0 }, param1[16] = { 0 }, param2[16] = { 0 };
	while (true)
	{
		printf("\n>>> ");
		scanf("%47[^\n]s", input);
		sscanf(input, "%15s%15s%15s", cmd, param1, param2);
		getchar();
		if (!strcmp("insert", cmd))
		{
			Account acc;
			printf(">>> (name, surname, balance): ");
			if (scanf("%s %s %lf", acc.name, acc.surname, &acc.balance) == 3)
				if (insert_customer(&acc))
					puts("the account has saved successful.\n");
				else
					puts("the account could not saved.\n");
			else
				puts("invalid input\n");
			getchar();
		}
		else if (!strcmp("delete", cmd))
		{
			unsigned int id = atol(param1);
			if (id > 0)
				if (select_customer(id))
					if (delete_customer(id))
						puts("the account has deleted successful.\n");
					else
						puts("the account could not delete.\n");
				else
					puts("the account could not find.\n");
			else
				puts("invalid input\n");
		}
		else if (!strcmp("add", cmd))
		{
			unsigned int id = atol(param1);
			double amount = atof(param2);
			if (id > 0 && amount)
				if (select_customer(id))
					if (add_money(id, amount))
						puts("the account has updated successful.\n");
					else
						puts("the account could not update.\n");
				else
					puts("the account could not find.\n");
			else
				puts("invalid input\n");
		}
		else if (!strcmp("select", cmd))
		{
			unsigned int id = atol(param1);
			if (id > 0)
			{
				Account* acc = select_customer(id);
				if (acc != NULL)
				{
					printf("%-16s%-16s%-16s%-16s\n", "ID", "NAME", "SURNAME", "BALANCE");
					printf("%-16u%-16s%-16s%-14.2lf\n", acc->accid, acc->name, acc->surname, acc->balance);
				}
				else
					puts("the account could not find.\n");
			}
			else
				puts("invalid input\n");
		}
		else if (!strcmp("list", cmd))
		{
			char wdel = atoi(param1);
			if (list_all(wdel))
				puts("accounts have listed.\n");
			else
				puts("accounts could not list.\n");
		}
		else if (!strcmp("exit", cmd))
			break;
		else if (!strcmp("help", cmd))
			puts("insert\ndelete [id]\nselect [id]\nadd [id] [amount]\nlist\nexit\n");
		else
			puts("invalid command.\n");
		cmd[0] = 0;
		param1[0] = 0;
		param2[0] = 0;
	}
	return 0;
}