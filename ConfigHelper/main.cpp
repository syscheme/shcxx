#include <ConfigHelper.h>
#include <list>
using namespace ZQ::common;
struct Job
{
    std::string name;
    std::string desc;
    int32 priority;
    static void structure(Config::Holder< Job > &holder)
    {
        holder.addDetail("", "name", &Job::name, NULL);
        holder.addDetail("", "desc", &Job::desc, "", Config::optReadOnly);
        holder.addDetail("", "priority", &Job::priority, NULL, Config::optReadOnly);
    }
};

struct Member
{
    std::string name;
    std::string email;
    typedef std::vector< Config::Holder< Job > > Jobs;
    Jobs jobs;
    static void structure(Config::Holder< Member > &holder)
    {
        holder.addDetail("", "name", &Member::name, NULL);
        holder.addDetail("detail", "mail", &Member::email, NULL, Config::optReadOnly, "email");
        holder.addDetail("job", &Member::readJob, &Member::registerJobs);
    }
    void readJob(XMLUtil::XmlNode node, const Preprocessor* hPP)
    {
        Config::Holder<Job> jobholder("name");
        jobholder.read(node, hPP);
        jobs.push_back(jobholder);
    }
    void registerJobs(const std::string &full_path)
    {
        for (Jobs::iterator it = jobs.begin(); it != jobs.end(); ++it)
        {
            it->snmpRegister(full_path);
        }
    }
};

struct Department
{
    char name[32];
    int32 memberCount;
    typedef std::list< Config::Holder<Member> > Members;
    Members members;
    static void structure(Config::Holder<Department> &holder)
    {
        holder.addDetail("", "name", (Config::Holder<Department>::PMem_CharArray)&Department::name, 32, NULL);
        holder.addDetail("members", "count", &Department::memberCount, NULL, Config::optReadOnly);
        holder.addDetail("members/member", &Department::readMember, &Department::registerMembers, Config::Range(0, -1), "member");
    }
    void readMember(XMLUtil::XmlNode node, const Preprocessor* hPP)
    {
        Config::Holder<Member> memholder("name");
        memholder.read(node, hPP);
        members.push_back(memholder);
    }
    void registerMembers(const std::string &full_path)
    {
        for(Members::iterator it = members.begin(); it != members.end(); ++it)
            it->snmpRegister(full_path);
    }
};

struct Company
{
    typedef std::list< Config::Holder<Department> > Departments;
    Departments departments;
    void readDepartment(XMLUtil::XmlNode node, const Preprocessor* hPP)
    {
        Config::Holder<Department> dptholder("name");
        dptholder.read(node, hPP);
        departments.push_front(dptholder);
    }
    void registerDepartments(const std::string &full_path)
    {
        for(Departments::iterator it = departments.begin(); it != departments.end(); ++it)
            it->snmpRegister(full_path);
    }
    static void structure(Config::Holder<Company> &holder)
    {
        holder.addDetail("department", &Company::readDepartment, &Company::registerDepartments);
    }
};

// print the config value after loading
void showConfig(const Company &cfg)
{
    for(Company::Departments::const_iterator it_d = cfg.departments.begin(); it_d != cfg.departments.end(); ++it_d)
    {
        printf("Department %s:\t", &(it_d->name));
        for(Department::Members::const_iterator it_m = it_d->members.begin(); it_m != it_d->members.end(); ++it_m)
        {
            printf("(%s, %s),\t", it_m->name.c_str(), it_m->email.c_str());
        }
        printf("\n");
    }
}
void test(const char* filepath, const char* dummySvc, int interval = -1)
{
    ZQ::common::Log log(Log::L_DEBUG);
    SNMPOpenSession(dummySvc, "TianShan", 2);
    Config::Loader< Company > company("");
    company.setLogger(&log);
    if(!company.load(filepath))
    {
        printf("failed to load %s", filepath);
        return;
    }
    showConfig(company);
    company.snmpRegister("ZQ");
    Sleep(interval);
    SNMPCloseSession();
}

int main(int argc, char* argv[])
{
    {
        Config::Loader< Company > company("");
        assert(!company.loadInFolder("C:\\TianShan"));
        assert(!company.load("C:\\TianShan\\"));
    }
    while(1)
    {
        test("CfgLoaderTest.xml", "ModTest");
    }
    return 0;
}