#include <iostream>
#include <vector>
#include <list>
#include <set>
void f(int = 5);
using namespace std;
int main()
{
    int n;
    cin>>n;
    vector<int> vv(n);
    for (int i = 0; i < n; i++) cin>>vv[i];
    vector<vector<int>> v(n, vector<int>(n));
    for (int i = 0; i < n; i++) {
        v[i][i] = vv[i];
        for (int j = i + 1; j < n; j++) {
            v[i][j] = v[i][j - 1] + vv[j];
        }
    }
    set<int> s;
    s.insert(v[0][n-1]);
    list<pair<int, int>> l;
    l.push_back({0, n - 1});
    cout<<(*s.rbegin())<<endl;
    int m;
    while (!l.empty()) {
        cin>>m;
        auto it = l.begin();
        while (it != l.end() && !(m >= (*it).first && m <= (*it).second)) it++;
        if (m == (*it).first && m == (*it).second) {
            l.erase(it);
            
            s.erase(vv[m]);
        } else if (m == (*it).first) {
            (*it).first++;
            s.erase(v[m][(*it).second]);
            s.insert(v[m + 1][(*it).second]);
        } else if (m == (*it).second) {
            (*it).second--;
            s.erase(v[(*it).first][m]);
            s.insert(v[(*it).first][m - 1]);
        } else {
            s.erase(v[(*it).first][(*it).second]);
            s.insert(v[(*it).first][m - 1]);
            s.insert(v[m + 1][(*it).second]);
            int temp = (*it).first;
            (*it).first = m + 1;
            l.insert(it, {temp, m - 1});
        }
        cout<<(*s.rbegin())<<endl;
    }
}