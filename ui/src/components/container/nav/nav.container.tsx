import { Menu } from 'antd';
import { Link } from 'react-router-dom';
import style from './nav.module.css';

const Nav = () => (
  <>
    <div className={style.nav}>
      <Menu
        defaultSelectedKeys={['all']}
        mode="horizontal"
      >
        <Menu.Item key="all">
          <Link to="/repos/all/1">All Repository</Link>
        </Menu.Item>
        <Menu.Item key="my">
          <Link to="/repos/my/1">My repository</Link>
        </Menu.Item>
      </Menu>
    </div>
  </>
);

export default Nav;
