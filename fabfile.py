from fabric.api import env, cd, run

PROJ_NAME = 'eeg-toolkit'


def env():
  env.user = 'ubuntu'
  env.hosts = ['128.52.175.133', '128.52.178.57']
  env.key_filename = '~/.ssh/id_rsa.pub'
  env.server_path = '~/%s' % PROJ_NAME
  env.python_path = '~/.virtualenvs/%s' % PROJ_NAME
  return


def prod_deploy():
  with cd(env.server_path):
    run('git pull --rebase origin master')
    run('make docker-run')
